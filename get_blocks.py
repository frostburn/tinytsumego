p_blocks = []
o_blocks = []

for key in range(3 ** 5):
    player = 0
    opponent = 0
    p = 1
    while key:
        if key % 3 == 1:
            player |= p
        elif key % 3 == 2:
            opponent |= p
        p <<= 1
        key /= 3
    p_blocks.append(player)
    o_blocks.append(opponent)

print p_blocks
print o_blocks