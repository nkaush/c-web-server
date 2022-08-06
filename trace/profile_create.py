lines = open('data', 'r').readlines()[2:-1] # open and skip empty lines
lines = [l[1:-1] for l in lines]          # remove leading space and trailing newline
lines = [l.rsplit(' ', 2) for l in lines] # split lines into func, library, and count

print("function,library,count")
for f, l, c in lines:
    print(f'"{f}",{l},{c}')