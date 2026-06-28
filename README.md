# sdlpal-lite

Stripped-down, DOS-only fork of SDLPAL (2017) with gameplay mods and a text-based hack system.

## Resource Files

Place these files in the same directory as the game data (alongside `WORD.DAT`, `DATA.MKF`, etc.):

### desc.txt — Object Descriptions

Displays descriptions for items and magic in their respective selection menus using the zpix small font.

**Format:**

```
; comment line (ignored)

+<hex_id>=<name>=<type>
description line 1
description line 2
-

```

- `+` starts an entry
  - `<hex_id>` — object ID in hexadecimal (e.g. `3d`, `128`)
  - `<name>` — display name (for reference only, from word.dat)
  - `<type>` — `M` for Magic, `I` for Item, `?` for Other
- Lines between `+` and `-` are the description text (UTF-8, multi-line)
- `-` ends the entry
- `;` lines are comments

**Example:**

```
+3d=觀音符=I
以觀音聖水書寫的靈符。
HP+150
-

+128=氣療術=M
我方單人HP+75
-
```

**Conversion tool:** `tools/desc_json2txt.py` converts from the old JSON format:

```bash
python3 tools/desc_json2txt.py desc.json word.dat -o desc.txt
```

---

### hack.txt — Text-Based Hack DSL

Allows modifying game data at load time or on demand via the debug menu.

**Format:**

```
; comment (ignored)

&GroupName
#INSTRUCTION arg1 arg2 ...
-

+GroupName
!Description text shown in menu
#INSTRUCTION arg1 arg2 ...
-
```

**Group prefixes:**

| Prefix | Behavior |
|--------|----------|
| `&` | Auto-execute on every game start / save load |
| `+` | Manual-only, selectable from debug menu (外典) |

**Lines within a group:**

| Prefix | Meaning |
|--------|---------|
| `!` | Description (shown in debug menu when selected, `+` groups only) |
| `#` | Instruction to execute |
| `;` | Comment (ignored) |
| `-` | End of group |

**Instructions:**

| Instruction | Description |
|-------------|-------------|
| `ADD_CASH <amount>` | Add (or subtract if negative) cash |
| `ADD_ITEM <obj_id> <count>` | Add items to inventory |
| `DEL_ITEM <obj_id> [count]` | Remove items (all if count omitted) |
| `ADD_MAGIC <player> <obj_id>` | Teach magic to party member (0-based index) |
| `CHANGE_MAGIC_DATA <id> <offset> <value>` | Modify MAGIC struct field by word offset |
| `CHANGE_OBJ <obj_id> <offset> <value>` | Modify OBJECT data by word offset (0-5) |
| `EDIT_SCRIPT <entry> <op> <a1> <a2> <a3>` | Overwrite a script entry |

All numeric arguments support decimal and hexadecimal (`0x` prefix).

**Example:**

```
; Auto-apply magic render mods on every load
&魔法渲染改
#CHANGE_MAGIC_DATA 0x65 11 16
#CHANGE_MAGIC_DATA 0x30 11 0x6000
-

; Manual: give 10000 gold (accessible from debug menu)
+土豪模式
!获得 10000 文
#ADD_CASH 10000
-
```

---

## Debug Menu

Press backtick (`` ` ``) to open the debug menu with these options:

- **尋蹤** — Trace/teleport submenu
- **氣凝** — Pawn shop (sell items)
- **藏真** — Random shop (+10000 gold)
- **俠影** — Party editor
- **試煉** — Battle picker
- **外典** — Hack menu (shows `+` groups from hack.txt)

## Building

```bash
make                        # Linux / macOS (default platform: unix)
make PLATFORM=win32         # Windows (via MSYS2/MinGW)
```

Requires SDL2 development libraries.
