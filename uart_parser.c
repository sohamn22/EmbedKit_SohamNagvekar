
#include <stdio.h>
#include <stdint.h>

#define SOF        0xAA
#define MAX_DATA   16
#define TIMEOUT_MS 50

typedef enum {
    WAIT_SOF,
    CMD,
    LEN,
    DATA,
    CHECKSUM
} State;

typedef struct {
    State state;
    uint8_t command;
    uint8_t length;
    uint8_t data[MAX_DATA];
    uint8_t index;
    uint8_t checksum;
    uint32_t last_time;
    uint32_t timeout;
} Parser;

// Reset parser
void reset(Parser *p)
{
    p->state = WAIT_SOF;
    p->index = 0;
    p->checksum = 0;
}

// Initialize parser
void init_parser(Parser *p, uint32_t timeout)
{
    reset(p);
    p->timeout = timeout;
    p->last_time = 0;
}

// Process incoming byte
int feed(Parser *p, uint8_t byte, uint32_t time)
{
    if (p->state != WAIT_SOF &&
        p->timeout != 0 &&
        time - p->last_time > p->timeout) {
        reset(p);
        return -2;
    }

    switch (p->state) {

        case WAIT_SOF:
            if (byte == SOF) {
                p->state = CMD;
                p->last_time = time;
            }
            break;

        case CMD:
            p->command = byte;
            p->checksum = byte;
            p->state = LEN;
            p->last_time = time;
            break;

        case LEN:
            p->length = byte;

            if (p->length > MAX_DATA) {
                reset(p);
                return 0;
            }

            p->checksum ^= byte;
            p->index = 0;
            p->last_time = time;

            if (p->length == 0)
                p->state = CHECKSUM;
            else
                p->state = DATA;
            break;

        case DATA:
            p->data[p->index++] = byte;
            p->checksum ^= byte;
            p->last_time = time;

            if (p->index == p->length)
                p->state = CHECKSUM;
            break;

        case CHECKSUM:
            p->last_time = time;

            if (byte == p->checksum) {
                p->state = WAIT_SOF;
                return 1;
            }

            reset(p);
            return -1;
    }

    return 0;
}

// Run parser test
void test(Parser *p, const uint8_t bytes[],
          const uint32_t times[], uint8_t count)
{
    for (uint8_t i = 0; i < count; i++) {

        int result = feed(p, bytes[i], times[i]);

        printf("t=%3ums byte=0x%02X -> ",
               times[i], bytes[i]);

        if (result == 1) {
            printf("FRAME OK CMD=0x%02X LEN=%u DATA=[",
                   p->command, p->length);

            for (uint8_t j = 0; j < p->length; j++) {
                printf("%02X", p->data[j]);

                if (j + 1 < p->length)
                    printf(" ");
            }

            printf("]\n");
        }
        else if (result == -1)
            printf("CHECKSUM ERROR -- reset\n");
        else if (result == -2)
            printf("TIMEOUT -- reset\n");
        else
            printf("receiving...\n");
    }
}

// Run all tests
int main(void)
{
    Parser parser;

uint8_t bytes1[] =
    {0xAA, 0x01, 0x03, 0x10, 0x20, 0x30, 0x02};

    uint32_t times1[] =
        {0, 5, 10, 15, 20, 25, 30};

    printf("\n--- TEST 1: CLEAN VALID FRAME ---\n");
    init_parser(&parser, TIMEOUT_MS);
    test(&parser, bytes1, times1, 7);

    uint8_t bytes2[] =
        {0xAA, 0x01, 0x03, 0x10,
         0x10, 0xAA, 0x05, 0x01, 0x7F, 0x7B};

    uint32_t times2[] =
        {0, 5, 10, 15, 200, 200, 205, 210, 215, 220};

    printf("\n--- TEST 2: TIMEOUT AND RECOVERY ---\n");
    init_parser(&parser, TIMEOUT_MS);
    test(&parser, bytes2, times2, 10);

    uint8_t bytes3[] =
        {0xAA, 0x03, 0x01, 0x55, 0x57,
         0xAA, 0x04, 0x02, 0xAA, 0xBB, 0x17};

    uint32_t times3[] =
        {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50};

    printf("\n--- TEST 3: TWO BACK-TO-BACK FRAMES ---\n");
    init_parser(&parser, TIMEOUT_MS);
    test(&parser, bytes3, times3, 11);

    printf("\n--- TEST 4: TIMEOUT DISABLED ---\n");
    init_parser(&parser, 0);
    test(&parser, bytes2, times2, 10);

    return 0;
}

