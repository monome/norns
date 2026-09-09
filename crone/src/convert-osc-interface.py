import re


#https://gist.github.com/constructor-igor/5f881c32403e3f313e6f
#http://stackoverflow.com/questions/4284991/parsing-nested-parentheses-in-python-grab-content-by-level
def parenthetic_contents(string):
    """Generate parenthesized contents in string as pairs (level, contents)."""
    stack = []
    for i, c in enumerate(string):
        if c == '(':
            stack.append(i)
        elif c == ')' and stack:
            start = stack.pop()
            yield (len(stack), string[start + 1: i])


infile = 'OscInterface.cpp'

output = ""

with open(infile) as f:
    txt = f.read()
    spl = txt.split('addServerMethod')
    for chunk in spl:
        pat = '\(\"(.*)\", \"(.*)\", \[\]\(lo_arg \*\*argv, int argc\)[\s*]{(.*)}\);[\s*]'
        p = re.compile(pat, re.DOTALL)
        m = p.match(chunk)
        if m:
            g = m.groups()
            oscpath = g[0]
            arglist = g[1]
            body = g[2]

            # build c++ parameters list from OSC arglist
            params = ""
            for i, ch in enumerate(arglist):
                if (ch == 'i'): params = params + ('int ')
                elif (ch == 'f'): params = params + ('float ')
                elif (ch == 's'): params = params + ('const char *')
                elif (ch == 'b'): params = params + ('void *')
                idx = str(i)
                argstr = 'argv['+idx+']->'+ch
                paramstr = 'arg'+idx
                body = body.replace(argstr, paramstr)
                params = params + paramstr + ", "

            funcname = "crone" + oscpath.replace("/", "_")
            if len(params) > 0:
                if params[-2] == ',':
                    params = params[:-2]
            body = body.replace("(void) argc;\n", "")
            body = body.replace("(void) argv;\n", "")
            body = body.replace("if (argc < 1) { return; }", "")
            body = body.replace("if (argc < 2) { return; }", "")
            body = body.replace("if (argc < 3) { return; }", "")
            func = f"void {funcname} ({params}) {{ {body} }}"
            output = output + func + '\n\n'
            print(func)

        print('\n\n')

outfile = "crone.h"
with open(outfile, "w") as f:
    f.write(output)
