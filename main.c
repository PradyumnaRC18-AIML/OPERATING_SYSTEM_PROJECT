#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <inttypes.h>

#define N 16
#define ENTRY_NUMBER 1024

typedef uint32_t unum;
typedef char* string;

typedef struct PAGE {
    unum PAGE_DIRECTORY_INDEX;
    unum PAGE_TABLE_INDEX;
    unum OFFSET;
} PAGE;

typedef struct TLB {
    unum VIRTUAL_PAGE_NUMBER;
    unum PHYSICAL_FRAME_NUMBER;
    int ValidBit;
} TLB;

typedef struct PTE {
    unum PHYSICAL_FRAME_NUMBER;
    int PRESENT_BIT;
} PAGE_TABLE_ENTRY;

typedef struct PT {
    PAGE_TABLE_ENTRY TABLE[ENTRY_NUMBER];
} PAGE_TABLE;

int PHYSICAL_FRAME_CHECK[ENTRY_NUMBER] = {0};
PAGE_TABLE* PAGE_DIRECTORY[ENTRY_NUMBER];
TLB tlb[N];
int TLB_HIT = 0;
int TLB_MISS = 0;
int PAGE_FAULT_COUNT = 0;
int REPLACEMENT_INDEX = 0;

unsigned char lower(char x);
int valid_address(string s);
unum hex_to_int(string address);
PAGE page_details(unum address);
void initialize_TLB(void);
unum lookup_TLB(unum VPN,int* not_found);
PAGE_TABLE* get_PAGE_TABLE(unum PAGE_DIRECTORY_INDEX);
void insert_TLB(unum VPN, unum PFN);
double effective_access_time(int TLB_lookup_time, int memory_access_time, int page_fault_service_time);

int get_int(string s) {
    printf("%s",s);
    int d;
    if(scanf("%d",&d) != 1) {
        printf("Invalid input. Please enter a valid integer.\n");
        exit(1);
    }
    return d;
}
string get_string(string s) {
    const int START = 512;
    printf("%s",s);
    string BUFFER = malloc(sizeof(char)*START);
    if(BUFFER == NULL) {
        return NULL;
    }
    int SIZE = START, length = 0, track;
    while((track = fgetc(stdin)) != '\n' && track != EOF) {
        if(length == SIZE-1) {
            SIZE = SIZE * 2;
            string temp_buff = realloc(BUFFER,sizeof(char)*SIZE);
            if(temp_buff == NULL) {
                free(BUFFER);
                return NULL;
            }
            BUFFER = temp_buff;
        }
        BUFFER[length++] = track;
    }
    BUFFER[length] = '\0';
    return BUFFER;
}

int main(int argc, string argv[]) {
    int len = argc - 1;
    if(len == 0) {
        printf("Usage: ./main address.....");
        return 1;
    }
    int TLB_lookup_time = get_int("Enter TLB lookup time (in ns): ");
    int memory_access_time = get_int("Enter memory access time (in ns): ");
    int page_fault_service_time = get_int("Enter page fault service time (in ms): ") * 1000000;
    if(TLB_lookup_time <= 0 || memory_access_time <= 0 || page_fault_service_time <= 0) {
        printf("All times must be positive integers.\n");
        return 1;
    }
    initialize_TLB();
    for(int main_i = 1; main_i < argc; main_i++) {
        string VIRTUAL_ADDRESS = argv[main_i];
        int validity = valid_address(VIRTUAL_ADDRESS);
        if(validity != 0) {
            return 1;
        }
        unum ADDRESS = hex_to_int(VIRTUAL_ADDRESS);
        PAGE page = page_details(ADDRESS);
        int flag;
        unum VPN = ADDRESS >> 12;
        unum PFN = lookup_TLB(VPN,&flag);
        if(flag == 1) {
            TLB_MISS++;
            PAGE_TABLE* TABLE = get_PAGE_TABLE(page.PAGE_DIRECTORY_INDEX);
            if(TABLE == NULL) return 1;
            PAGE_TABLE_ENTRY* value = &(TABLE->TABLE[page.PAGE_TABLE_INDEX]);
            if(value->PRESENT_BIT == 1) {
                PFN = value->PHYSICAL_FRAME_NUMBER;
            }
            else {
                PAGE_FAULT_COUNT++;
                int COUNT = 0;
                for(int i = 0; i < ENTRY_NUMBER; i++) {
                    if(PHYSICAL_FRAME_CHECK[i] == 0) {
                        PFN = i;
                        value->PHYSICAL_FRAME_NUMBER = PFN;
                        value->PRESENT_BIT = 1;
                        PHYSICAL_FRAME_CHECK[i] = 1;
                        break;
                    }
                    COUNT++;
                }
                if(COUNT == ENTRY_NUMBER) return 1;
            }
            insert_TLB(VPN,PFN);
        }
        else {
            TLB_HIT++;
        }
        unum PHYSICAL_ADDRESS = (PFN << 12) | page.OFFSET;
        float HIT_RATE = 0;
        float MISS_RATE = 0;
        if(TLB_HIT + TLB_MISS != 0) {
            HIT_RATE = ((float)TLB_HIT / (TLB_HIT + TLB_MISS)) * 100.0;
            MISS_RATE = ((float)TLB_MISS / (TLB_HIT + TLB_MISS)) * 100.0;
        }
        printf("\n");
        printf("============================================\n");
        printf("       VIRTUAL MEMORY ADDRESS TRANSLATION   \n");
        printf("============================================\n");

        printf("\n--- ADDRESS INFORMATION ---\n");
        printf("Virtual Address       : 0x%08" PRIX32 "\n", ADDRESS);
        printf("Virtual Page Number   : %" PRIu32 "\n", VPN);
        printf("Page Directory Index  : %" PRIu32 "\n", page.PAGE_DIRECTORY_INDEX);
        printf("Page Table Index      : %" PRIu32 "\n", page.PAGE_TABLE_INDEX);
        printf("Page Offset           : %" PRIu32 "\n", page.OFFSET);

        printf("\n--- TRANSLATION RESULT ---\n");
        printf("TLB Status            : %s\n", flag == 1 ? "MISS" : "HIT");
        printf("Physical Frame Number : %" PRIu32 "\n", PFN);
        printf("Physical Address      : 0x%08" PRIX32 "\n", PHYSICAL_ADDRESS);

        printf("\n--- MEMORY STATISTICS ---\n");
        printf("TLB Hits              : %d\n", TLB_HIT);
        printf("TLB Misses            : %d\n", TLB_MISS);
        printf("Page Faults           : %d\n", PAGE_FAULT_COUNT);
        printf("TLB Hit Rate          : %.2f%%\n", HIT_RATE);
        printf("TLB Miss Rate         : %.2f%%\n", MISS_RATE);
        printf("Effective Access Time : %.2f ns\n", effective_access_time(TLB_lookup_time, memory_access_time, page_fault_service_time));
        printf("============================================\n");
    }
    return 0;
}

unsigned char lower(char x) {
    return tolower((unsigned char) x);
}
int valid_address(string s) {
    if(s == NULL) {
        printf("can't handle a NULL value\n");
        return -1;
    }
    int length = strlen(s);
    int new_pos = 0;
    int new_length = length;
    if(length == 0) {
        printf("Address is empty\n");
        return -1;
    }
    if(length >= 2 && s[0] == '0' && lower(s[1]) == 'x') {
        if(length == 2) {
            printf("Address is still empty\n");
            return -1;
        }
        new_pos = new_pos + 2;
        new_length = new_length - 2;
    }
    if(!(new_length >= 1 && new_length <=8)) {
        printf("Address length must be between 1 to 8\n");
        return -2;
    }
    for(int i = new_pos; i < length; i++) {
        int num_ascii = s[i] - '0';
        int alpha_ascii = lower(s[i]) - 'a';
        if(!((0 <= num_ascii && num_ascii <= 9) || (0 <= alpha_ascii && alpha_ascii <= 5))) {
            printf("Invalid Allowed Hexadecimal digit : 0-9 and A-F\n");
            return 1;
        }
    }
    return 0;
}
unum hex_to_int(string address) {
    int pos = 0;
    if(address[0] == '0' && lower(address[1]) == 'x') {
        pos += 2;
    }
    int length = strlen(address);
    unum number = 0;
    for(int i = pos; i < length; i++) {
        if('0' <= address[i] && address[i] <= '9') {
            number = number * 16 + (address[i] - '0');
        }
        else{
            number = number * 16 + ((lower(address[i]) - 'a') + 10);
        }
    }
    return number;
}
PAGE page_details(unum address) {
    PAGE page;
    unum mask_bit_10 = 0x3FF;
    unum mask_bit_full = 0xFFF;
    page.PAGE_DIRECTORY_INDEX = (address >> 22) & mask_bit_10;
    page.PAGE_TABLE_INDEX = (address >> 12) & mask_bit_10;
    page.OFFSET = address & mask_bit_full;
    return page;
}

void initialize_TLB(void) {
    REPLACEMENT_INDEX = 0;
    for(int i = 0; i < N; i++) {
        tlb[i].PHYSICAL_FRAME_NUMBER = 0;
        tlb[i].VIRTUAL_PAGE_NUMBER = 0;
        tlb[i].ValidBit = 0;
    }
}

unum lookup_TLB(unum VPN,int* not_found) {
    *not_found = 0;
    for(int i = 0; i < N; i++) {
        if(tlb[i].VIRTUAL_PAGE_NUMBER == VPN && tlb[i].ValidBit) {
            return tlb[i].PHYSICAL_FRAME_NUMBER;
        }
    }
    *not_found = 1;
    return 0;
}

PAGE_TABLE* get_PAGE_TABLE(unum PAGE_DIRECTORY_INDEX) {
    if(PAGE_DIRECTORY[PAGE_DIRECTORY_INDEX] == NULL) {
        PAGE_TABLE* new_PAGE_TABLE = calloc(1,sizeof(PAGE_TABLE));
        if(new_PAGE_TABLE == NULL) return NULL;
        PAGE_DIRECTORY[PAGE_DIRECTORY_INDEX] = new_PAGE_TABLE;
    }
    return PAGE_DIRECTORY[PAGE_DIRECTORY_INDEX];
}

void insert_TLB(unum VPN, unum PFN) {
    for(int i = 0; i < N; i++) {
        if(tlb[i].ValidBit == 0) {
            tlb[i].PHYSICAL_FRAME_NUMBER = PFN;
            tlb[i].VIRTUAL_PAGE_NUMBER = VPN;
            tlb[i].ValidBit = 1;
            return;
        }
    }
    tlb[REPLACEMENT_INDEX].VIRTUAL_PAGE_NUMBER = VPN;
    tlb[REPLACEMENT_INDEX].PHYSICAL_FRAME_NUMBER = PFN;
    tlb[REPLACEMENT_INDEX].ValidBit = 1;
    REPLACEMENT_INDEX = (REPLACEMENT_INDEX+1)%N;
}


double effective_access_time(int TLB_lookup_time,int memory_access_time,int page_fault_service_time) {
    int TOTAL = TLB_HIT + TLB_MISS;
    if(TOTAL == 0) {
        printf("Error: No memory accesses recorded.\n");
        return 0.0;
    }
    if(PAGE_FAULT_COUNT > TLB_MISS) {
        printf("Error: Invalid memory statistics.\n");
        return 0.0;
    }
    double expression = (double)TLB_HIT * (TLB_lookup_time + memory_access_time) + (double)(TLB_MISS - PAGE_FAULT_COUNT) * (TLB_lookup_time + 3.0 * memory_access_time) + (double)PAGE_FAULT_COUNT * (TLB_lookup_time + 3.0 * memory_access_time + page_fault_service_time);
    return expression / TOTAL;
}