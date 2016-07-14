#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

/* Public domain code for JKISS RNG */
// http://www0.cs.ucl.ac.uk/staff/d.jones/GoodPracticeRNG.pdf

typedef struct jkiss
{
    unsigned int x, y, z, c;
} jkiss;

#define JKISS_SEED ({123456789, 987654321, 43219876, 6543217})

unsigned int devrand(void)
{
    int fn;
    unsigned int r;
    fn = open("/dev/urandom", O_RDONLY);
    if (fn == -1)
        exit(-1); /* Failed! */
    if (read(fn, &r, 4) != 4)
        exit(-1); /* Failed! */
    close(fn);
    return r;
}

/* Initialise KISS generator using /dev/urandom */
void jkiss_init(jkiss *j)
{
    j->x = devrand();
    while (!(j->y = devrand())); /* y must not be zero! */
    j->z = devrand();
    /* We don’t really need to set c as well but let's anyway… */
    /* NOTE: offset c by 1 to avoid z=c=0 */
    j->c = devrand() % 698769068 + 1; /* Should be less than 698769069 */
}

/* Try to scramble the generator for parallel iteration based on the thread id alone */
// void jkiss_scramble(jkiss *j, int thread_id)
// {
//     return;
// }

unsigned int jkiss_step(jkiss *j)
{
    unsigned long long t;
    j->x = 314527869 * j->x + 1234567;
    j->y ^= j->y << 5; j->y ^= j->y >> 7; j->y ^= j->y << 22;
    t = 4294584393ULL * j->z + j->c;
    j->c = t >> 32;
    j->z = t;
    return j->x + j->y + j->z;
}

#ifndef MAIN
int main()
{
    jkiss j = jkiss (JKISS_SEED);
    printf("Default iteration: Should always be the same.\n");
    for (int i = 0; i < 5; i++) {
        printf("%u\n", jkiss_step(&j));
    }
    printf("Devrand iteration: Should be different each time.\n");
    jkiss_init(&j);
    for (int i = 0; i < 5; i++) {
        printf("%u\n", jkiss_step(&j));
    }
}
#endif
