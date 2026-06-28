#!/usr/bin/env python3
"""Convert desc.json to desc.txt format for sdlpal-lite.

Usage: python3 desc_json2txt.py <desc.json> [word.dat] [-o output.txt]

If word.dat is provided, object names are filled from it (Big5, 10 bytes/word).
"""

import json, sys, os

def load_words(word_dat_path):
    with open(word_dat_path, 'rb') as f:
        data = f.read()
    words = {}
    word_len = 10
    for i in range(len(data) // word_len):
        raw = data[i*word_len:(i+1)*word_len].rstrip(b' \x00')
        try:
            words[i] = raw.decode('big5').strip()
        except:
            pass
    return words

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <desc.json> [word.dat] [-o output.txt]")
        sys.exit(1)

    json_path = sys.argv[1]
    word_dat_path = None
    output_path = None

    args = sys.argv[2:]
    i = 0
    while i < len(args):
        if args[i] == '-o' and i + 1 < len(args):
            output_path = args[i + 1]
            i += 2
        else:
            word_dat_path = args[i]
            i += 1

    with open(json_path, 'r', encoding='utf-8') as f:
        data = json.load(f)

    words = load_words(word_dat_path) if word_dat_path else {}

    if output_path is None:
        output_path = os.path.splitext(json_path)[0] + '.txt'

    lines = [
        '; desc.txt - Object descriptions for sdlpal-lite',
        '; Converted from ' + os.path.basename(json_path),
        ';',
        '; Format: +<hex_id>=<name>=<type>',
        ';   description lines',
        ';   -',
        '; type: M=Magic, I=Item, ?=Other',
        '',
    ]

    for key, desc in data.items():
        parts = key.split('|')
        hex_id = parts[0]
        obj_type = parts[1] if len(parts) > 1 else '?'
        wid = int(hex_id, 16)
        name = words.get(wid, '')
        lines.append(f'+{hex_id}={name}={obj_type}')
        for dl in desc.split('\n'):
            lines.append(dl)
        lines.append('-')
        lines.append('')

    with open(output_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))

    print(f'Converted {len(data)} entries -> {output_path}')

if __name__ == '__main__':
    main()
