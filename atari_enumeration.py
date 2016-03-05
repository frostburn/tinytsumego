def is_from_alternate_play(key, N):
    p = 0
    o = 0
    while key:
        if key % 3 == 1:
            p += 1
        elif key % 3 == 2:
            o += 1
        key //= 3
    return 0 <= o - p <= 1 and o + p < N

for N in range(5):
    print sum(is_from_alternate_play(k, N) for k in xrange(3 ** N))

print "*****"

def ceil_div(a, b):
    return -(-a // b)

def scan(b, start=0):
    for e in b:
        if e:
            break
        start += 1
    return start

def combination_size(count, space):
    if count:
        return sum(combination_size(count - 1, space - 1 - i) for i in range(space))
    return 1

c = combination_size

def n_choose_k(n, k):
    num = 1
    denom = 1
    for i in xrange(k):
        num *= n - i
        denom *= k - i
    r = num / denom
    # print (n, k, r, c(k, n))
    return r

combination_size = lambda c, s: n_choose_k(s, c)

def combination_key(b):
    if not b:
        return 0
    count = sum(b)
    space = len(b)
    index = scan(b)
    offset = sum(combination_size(count - 1, space - 1 - i) for i in range(index))
    return offset + combination_key(b[index + 1:])

def combination_from_key(k, count, space):
    assert(k >= 0)
    if not count:
        return [0] * space
    b = []
    for index in range(space):
        s = combination_size(count - 1, space - 1 - index)
        if k < s:
            return b + [1] + combination_from_key(k, count - 1, space - 1 - index)
        b.append(0)
        k -= s
    assert(False)

def count_size(count, space):
    o_count = ceil_div(count, 2)
    return combination_size(o_count, space) * combination_size(count // 2, space - o_count)

def to_key(b):
    count = sum(map(bool, b))
    o = [e == 2 for e in b]
    p = [e for e in b if e < 2]
    offset = sum(count_size(count - 1 - i, len(b)) for i in range(count))
    assert(sum(o) == ceil_div(count, 2))
    assert(sum(p) == count // 2)
    o_key = combination_key(o)
    p_key = combination_key(p)
    return offset + p_key + combination_size(sum(p), len(p)) * o_key

def key_size(space):
    s = 0
    for count in range(space + 1):
        s += count_size(count, space)
    return s

def from_key(k, space):
    for count in range(space + 1):
        s = count_size(count, space)
        if k < s:
            o_count = ceil_div(count, 2)
            p_count = count // 2
            m = combination_size(p_count, space - o_count)
            p_key = k % m
            o_key = k // m
            o = combination_from_key(o_key, o_count, space)
            p = combination_from_key(p_key, p_count, space - o_count)
            b = []
            for e in o:
                if e:
                    b.append(2)
                else:
                    b.append(p.pop(0))
            return b
        k -= s

print combination_size(2, 3)
print combination_key([1, 1, 0])
print combination_key([1, 0, 1])
print combination_key([0, 1, 1])
print combination_from_key(0, 2, 3)
print combination_from_key(1, 2, 3)
print combination_from_key(2, 2, 3)
print "-----"
print combination_size(2, 4)
print combination_key([1, 1, 0, 0])
print combination_key([1, 0, 1, 0])
print combination_key([1, 0, 0, 1])
print combination_key([0, 1, 1, 0])
print combination_key([0, 1, 0, 1])
print combination_key([0, 0, 1, 1])
print combination_from_key(0, 2, 4)
print combination_from_key(1, 2, 4)
print combination_from_key(2, 2, 4)
print combination_from_key(3, 2, 4)
print combination_from_key(4, 2, 4)
print combination_from_key(5, 2, 4)
print "+++++"
print to_key([0, 0, 0])
print to_key([2, 0, 0])
print to_key([0, 2, 0])
print to_key([0, 0, 2])
print to_key([2, 1, 0])
print to_key([2, 0, 1])
print to_key([1, 2, 0])
print to_key([0, 2, 1])
print to_key([1, 0, 2])
print to_key([0, 1, 2])
print to_key([2, 2, 1])
print to_key([2, 1, 2])
print to_key([1, 2, 2])
for k in range(key_size(3)):
    print from_key(k, 3)

for k in range(key_size(4)):
    print from_key(k, 4)

table = [0] * (14 * 26)
for k in range(14):
    for n in range(26):
        table[n + 26 * k] = n_choose_k(n, k);
print table
