# Makefile for sdlpal-lite (DOS version only)

SRCDIR = src
TARGET = sdlpal
BUILDDIR = build

HOST =
PLATFORM = unix

DEPFLAGS = -MT $@ -MMD -MP -MF $(BUILDDIR)/$*.Td

CFILES = $(wildcard $(SRCDIR)/adplug/*.c) $(wildcard $(SRCDIR)/*.c)
CPPFILES = $(wildcard $(SRCDIR)/adplug/*.cpp) $(wildcard $(SRCDIR)/*.cpp) $(SRCDIR)/$(PLATFORM)/$(PLATFORM).cpp
OBJFILES = $(addprefix $(BUILDDIR)/, $(notdir $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)))
DEPFILES = $(OBJFILES:.o=.d)
SDL_CONFIG = sdl2-config

CC = $(HOST)gcc
CXX = $(HOST)g++
CCFLAGS = `$(SDL_CONFIG) --cflags` -g -Wall -O2 -fno-strict-aliasing \
	-I$(SRCDIR)/$(PLATFORM) -I$(SRCDIR) -DPAL_HAS_PLATFORM_SPECIFIC_UTILS
CXXFLAGS = $(CCFLAGS) -std=c++11
CFLAGS = $(CCFLAGS) -std=gnu99
LDFLAGS = `$(SDL_CONFIG) --libs` -lm -pthread

POSTCOMPILE = @mv -f $(BUILDDIR)/$*.Td $(BUILDDIR)/$*.d && touch $@

.PHONY : all clean

all: $(TARGET)

$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

$(TARGET): $(OBJFILES)
	@echo [LD] $@
	@$(CXX) $^ -o $@ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(BUILDDIR)/%.d | $(BUILDDIR)
	@echo [CC] $<
	@$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@
	$(POSTCOMPILE)

$(BUILDDIR)/%.o: $(SRCDIR)/adplug/%.c $(BUILDDIR)/%.d | $(BUILDDIR)
	@echo [CC] $<
	@$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@
	$(POSTCOMPILE)

$(BUILDDIR)/%.o: $(SRCDIR)/adplug/%.cpp $(BUILDDIR)/%.d | $(BUILDDIR)
	@echo [CC] $<
	@$(CXX) $(DEPFLAGS) $(CXXFLAGS) -c $< -o $@
	$(POSTCOMPILE)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp $(BUILDDIR)/%.d | $(BUILDDIR)
	@echo [CC] $<
	@$(CXX) $(DEPFLAGS) $(CXXFLAGS) -c $< -o $@
	$(POSTCOMPILE)

$(BUILDDIR)/%.o: $(SRCDIR)/$(PLATFORM)/%.cpp $(BUILDDIR)/%.d | $(BUILDDIR)
	@echo [CC] $<
	@$(CXX) $(DEPFLAGS) $(CXXFLAGS) -c $< -o $@
	$(POSTCOMPILE)

clean:
	-rm -rf $(TARGET) $(BUILDDIR)

$(BUILDDIR)/%.d: ;
.PRECIOUS: $(BUILDDIR)/%.d

-include $(DEPFILES)
