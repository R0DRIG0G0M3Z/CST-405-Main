#!/usr/bin/env python3
"""Project 2 regressions: execute real generated MIPS and assert diagnostics.

Both optimized and -O0 output are checked against expected program behavior.
A deterministic random corpus stresses reassignment, temporary reuse and
register spilling. SPIM banner text is never mistaken for program output.
"""
from pathlib import Path
import random
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
COMPILER = ROOT / 'minicompiler'
checks = 0

def run(args):
    return subprocess.run(args, cwd=ROOT, text=True, capture_output=True, timeout=15)

def wrap(value):
    return (value + 2**31) % 2**32 - 2**31

def valid(source, expected, label):
    global checks
    for mode in ([], ['-O0']):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp); src = p/'source.cm'; out = p/'program.s'
            src.write_text(source)
            result = run([str(COMPILER),str(src),str(out),'-q',*mode])
            assert result.returncode == 0, (label, mode, result.stdout, result.stderr)
            assert 'COMPILATION SUCCESSFUL' in result.stdout
            for path in (out, p/'program.tac', p/'program.optimized.tac'):
                assert path.is_file() and path.stat().st_size > 0, (label,path)
            jar=os.environ.get('MARS_JAR')
            simulator=['java','-Djava.awt.headless=true','-jar',jar,'nc','sm','ae1','se1',str(out)] if jar else ['spim','-file',str(out)]
            result = run(simulator)
            assert result.returncode == 0 and not result.stderr, (label,result)
            actual = [int(line) for line in result.stdout.splitlines()
                      if re.fullmatch(r'-?\d+',line.strip())]
            assert actual == expected, (label,mode,actual,expected,result.stdout)
            checks += 1

def invalid(source, message, label):
    global checks
    with tempfile.TemporaryDirectory() as tmp:
        p=Path(tmp); src=p/'bad.cm'; out=p/'bad.s'; src.write_text(source)
        result=run([str(COMPILER),str(src),str(out),'-q'])
        assert result.returncode != 0, (label,result.stdout)
        assert message in result.stderr, (label,result.stderr)
        assert 'COMPILATION SUCCESSFUL' not in result.stdout
        assert not any(p.glob('*.s')) and not any(p.glob('*.tac')), label
        checks += 1

for name,expected in [('t2_01_basics.cm',[5,10,18]),('t2_02_chained.cm',[6,21]),
                      ('t2_03_comments.cm',[42])]:
    valid((ROOT/'tests'/name).read_text(),expected,name)
valid('int total; int tigers; total=1+2; tigers=total+total+3; print(tigers);', [9], 'names beginning with t')
valid('int x; print(x); x=x+1; print(x);', [0,1], 'zero initialization')
valid('int x; x=2147483647+1; print(x); x=x+2147483647; print(x);', [-2147483648,-1], '32-bit wrap')
valid('int printValue;\r\nprintValue=40+2;\r\nprint(printValue);', [42], 'CRLF and keyword prefixes')
valid('print('+ '+'.join(['1']*200)+');', [200], 'temporary reuse in long expression')
valid(' '.join('int v%d; v%d=%d;'%(i,i,i) for i in range(128))+' print(v0+v127);', [127], 'symbol capacity')

invalid((ROOT/'tests/t2_04_errors_undeclared.cm').read_text(), "line 8, column 7: 'ghost'", 'supplied semantic failure')
invalid((ROOT/'tests/t2_05_errors_syntax.cm').read_text(), 'line 8, column 1', 'supplied syntax failure')
for source,message,label in [
 ('int x;\nint x;', 'line 2, column 5', 'duplicate'),
 ('x=1;', "assignment target 'x'", 'undeclared target'),
 ('int x; x=ghost+missing;', "'ghost'", 'undeclared expression'),
 ('int t0;', 'Reserved Identifier', 'reserved temp'),
 ('int L15;', 'Reserved Identifier', 'reserved label'),
 ('int x; x=1; @ print(x);', "unknown character '@'", 'invalid character'),
 ('int x; /* never closed', 'line 1, column 8: unterminated', 'unterminated comment'),
 ('print(2147483648);', 'integer literal exceeds', 'oversized integer'),
 ('int '+ 'x'*64 + ';', 'identifier exceeds', 'oversized name'),
 ('int x; x=1', "expecting", 'missing final semicolon'),
 ('print(1;', 'Syntax error', 'missing parenthesis'),
 ('print(1-2);', "unknown character '-'", 'unsupported arithmetic'),
 ('int x=1;', 'Syntax error', 'unsupported initializer'),
 ('', 'Syntax error', 'empty program'),
 ('int x; x=; print();', 'Syntax error', 'multiple syntax errors'),
 (' '.join('int v%d;'%i for i in range(129)), 'scope limit', 'too many variables')
]: invalid(source,message,label)

# Exercise numerous cache evictions and reused temporaries against a tiny
# independent interpreter for this addition-only grammar.
rng=random.Random(405)
for trial in range(40):
    names=['v%d'%i for i in range(18)]
    values=dict.fromkeys(names,0)
    lines=['int %s;'%name for name in names]; expected=[]
    for _ in range(40):
        target=rng.choice(names)
        operands=[rng.choice(names) if rng.random()<.65 else str(rng.randrange(1000))
                  for _ in range(rng.randrange(1,8))]
        values[target]=wrap(sum(values[x] if x in values else int(x) for x in operands))
        lines.append('%s=%s;'%(target,'+'.join(operands)))
        if rng.random()<.3:
            lines.append('print(%s);'%target); expected.append(values[target])
    lines.extend('print(%s);'%name for name in names)
    expected.extend(values.values())
    valid('\n'.join(lines),expected,'random corpus %d'%trial)

with tempfile.TemporaryDirectory() as tmp:
    p=Path(tmp); src=p/'src.cm'; src.write_text('print(42);')
    r=run([str(COMPILER),str(src),str(src),'-q'])
    assert r.returncode != 0 and src.read_text()=='print(42);'
    r=run([str(COMPILER),str(src),str(p/'missing/out.s'),'-q'])
    assert r.returncode != 0 and 'COMPILATION SUCCESSFUL' not in r.stdout
    checks += 2
print('PASS: %d checks using %s; real MIPS output matches expected values in optimized and -O0 modes.'%(checks,'MARS' if os.environ.get('MARS_JAR') else 'SPIM'))
