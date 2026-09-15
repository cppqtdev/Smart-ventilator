#!/usr/bin/env python3
"""Cross-checks declarations in a header against definitions in its .cpp.

A light substitute for a compiler on a machine with no Qt toolchain. It
catches the error a fresh header edit most often produces - a member declared
and never defined - which otherwise only surfaces at link time.

It tracks brace depth so a free function declared after a struct is not
attributed to that struct, and it accepts either a qualified
(Class::name / Namespace::name) or an unqualified definition.

Deliberately conservative: pure virtuals, defaulted, deleted and inline
definitions are skipped, as is anything it cannot parse. A clean run means
"nothing obviously missing", not "this compiles".
"""
from __future__ import annotations

import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

DECL = re.compile(
    r'^\s*(?:Q_INVOKABLE\s+)?(?:explicit\s+|static\s+|virtual\s+|inline\s+|constexpr\s+)*'
    r'(?:[\w:<>,\s\*&]+?\s+)?'
    r'(~?\w+)\s*\([^;{]*\)\s*'
    r'(?:const\s*)?(?:noexcept\s*)?(?:override\s*)?(?:final\s*)?;\s*$')

SKIP = re.compile(r'=\s*(0|default|delete)\s*;|Q_PROPERTY|Q_OBJECT|Q_ENUM|'
                  r'Q_DISABLE|typedef|using\s|^\s*friend\b')

CONTROL_WORDS = {'if', 'for', 'while', 'switch', 'return', 'catch', 'sizeof'}


def strip_comments(text: str) -> str:
    text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
    return re.sub(r'//[^\n]*', '', text)


def declarations(header: str):
    """Yields declared function names, ignoring signals and inline bodies."""
    text = strip_comments(open(header, encoding='utf-8').read())
    names = []
    in_signals = False
    signals_depth = -1
    depth = 0
    for raw in text.split('\n'):
        if re.match(r'^\s*(signals|Q_SIGNALS)\s*:', raw):
            in_signals = True
            signals_depth = depth
            depth += raw.count('{') - raw.count('}')
            continue
        if in_signals and re.match(r'^\s*(public|private|protected)\s*(slots|Q_SLOTS)?\s*:', raw):
            in_signals = False
        opened = raw.count('{')
        closed = raw.count('}')
        if in_signals and depth - closed < signals_depth:
            in_signals = False
        if not in_signals and not SKIP.search(raw):
            m = DECL.match(raw)
            if m and m.group(1) not in CONTROL_WORDS:
                names.append(m.group(1))
        depth += opened - closed
    return names


def definitions(source: str):
    """Every function name defined in the translation unit."""
    text = strip_comments(open(source, encoding='utf-8').read())
    qualified = set(n for _, n in re.findall(r'(\w+)\s*::\s*(~?\w+)\s*\(', text))
    # Namespace-scope definitions: a signature followed by an opening brace.
    free = set(re.findall(r'^\s*(?:[\w:<>,\s\*&]+?\s+)?(\w+)\s*\([^;]*\)\s*'
                          r'(?:const\s*)?(?:noexcept\s*)?\{', text, re.M))
    return qualified | free


def main() -> int:
    problems = []
    for base, dirs, files in os.walk(os.path.join(ROOT, 'src')):
        for f in files:
            if not f.endswith('.h'):
                continue
            header = os.path.join(base, f)
            stem = f[:-2]
            parts = header.split(os.sep)
            candidates = [os.path.join(base, stem + '.cpp')]
            if 'include' in parts:
                libroot = os.sep.join(parts[:parts.index('include')])
                candidates.append(os.path.join(libroot, 'src', stem + '.cpp'))

            source = next((c for c in candidates if os.path.isfile(c)), None)
            if not source:
                continue

            defined = definitions(source)
            for name in declarations(header):
                if name not in defined:
                    problems.append('%s: %s() declared but not defined in %s'
                                    % (os.path.relpath(header, ROOT), name,
                                       os.path.relpath(source, ROOT)))

    for p in problems:
        print(p)
    print('\n%d potentially missing definition(s)' % len(problems))
    return 1 if problems else 0


if __name__ == '__main__':
    raise SystemExit(main())
