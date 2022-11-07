from os import system

NCPU = 8
pages = [0 for i in range(NCPU)]

with open("temp.txt", 'r') as f:
  lines = [line.rstrip().split(' ') for line in f.readlines()]
lines = lines[:-12]
while(lines[0][0] != 'f'):
  lines = lines[1:]

def show():
  for i in range(len(pages)):
    print("CPU", i, "holds", pages[i])

for i in range(len(lines)):
  id = int(lines[i][1])
  if(lines[i][0] == 'a'):
    pages[id] -= 1
  elif(lines[i][0] == 'f'):
    pages[id] += 1
  else:
    continue
  if(pages[id] != int(lines[i][2])):
    show()
    print("at line", i, "got pages", int(lines[i][2]), "but it should be", pages[id])
    m = input()
  else:
    print(i+1, "/", len(lines))
        
    
