# Virtual-to-Physical Address Translation Simulator

A C-based simulation of virtual-to-physical address translation using a Two-Level Page Table and a Translation Lookaside Buffer (TLB).

## 1. Project Overview

### Problem Statement

Simulate address translation from virtual to physical addresses using a two-level page table and a TLB. Use bitwise operations to extract page numbers and offsets, and report TLB hit rate, page faults, and effective access time.

### Objective

The objective is to understand how an operating system translates virtual addresses into physical addresses.

The simulator demonstrates:

- Two-level paging.
- Page Directory and Page Tables.
- Translation Lookaside Buffer (TLB).
- Bitwise address decomposition.
- Physical frame allocation.
- Page fault detection.
- TLB hit/miss tracking.
- Effective Access Time (EAT).

This is a software simulation written in C. It does not implement actual hardware or modify the memory management of the host operating system.

---

## 2. Understanding the Core Concepts

### 2.1 Virtual Memory

Virtual memory is a memory-management technique that provides processes with their own virtual address spaces.

A program generates virtual addresses when accessing memory. These addresses must be translated into physical addresses to access RAM.

### 2.2 Paging

Paging divides virtual memory into fixed-sized blocks called **pages**.

Physical memory is divided into equally sized blocks called **frames**.

A virtual page can be mapped to any available physical frame.

In this project, both page size and frame size are 4 KB.

### 2.3 Two-Level Page Table

The two-level Page Table consists of:

**Page Directory (First Level)**

- Contains 1024 entries.
- Each entry points to a Page Table.
- Page Tables are allocated dynamically when needed.

**Page Table (Second Level)**

- Each Page Table contains 1024 Page Table Entries (PTEs).
- Each PTE stores a Physical Frame Number (PFN).
- Each PTE also contains a Present Bit.

### 2.4 Translation Lookaside Buffer (TLB)

The TLB is a small, fast cache that stores recently used address translations.

It stores:

`Virtual Page Number (VPN) → Physical Frame Number (PFN)`

**TLB Hit:** The requested VPN is found in the TLB.

**TLB Miss:** The requested VPN is not found in the TLB, so the Page Table must be checked.

A TLB miss does not necessarily mean a page fault.

### 2.5 Page Fault

A page fault occurs when the requested virtual page is not currently present in physical memory.

In this simulation, the program searches for a free physical frame, updates the corresponding PTE and inserts the translation into the TLB.

---

## 3. System Configuration

| Parameter | Configuration |
|---|---|
| Virtual Address Size | 32 bits |
| Page Size | 4 KB |
| Frame Size | 4 KB |
| Page Directory Entries | 1024 |
| Page Table Entries | 1024 |
| TLB Entries | 16 |
| Physical Frames | 1024 |
| Simulated Physical Memory | 4 MB |
| Address Format | Hexadecimal |

### Virtual Address Breakdown

A 32-bit virtual address is divided into three components:

| Component | Bits | Purpose |
|---|---:|---|
| Page Directory Index (PDI) | 10 | Selects a Page Directory entry |
| Page Table Index (PTI) | 10 | Selects a Page Table entry |
| Offset | 12 | Identifies a byte within the page |

Therefore:

`32 = 10 + 10 + 12`

### Bitwise Address Extraction

The simulator uses the following operations:

**Page Directory Index**

`PDI = (ADDRESS >> 22) & 0x3FF`

**Page Table Index**

`PTI = (ADDRESS >> 12) & 0x3FF`

**Offset**

`OFFSET = ADDRESS & 0xFFF`

**Virtual Page Number**

`VPN = ADDRESS >> 12`

Here, `0x3FF` selects 10 bits, while `0xFFF` selects 12 bits.

---

## 4. Understanding the Data Structures

This section explains the purpose of every structure used in `main.c`.

### 4.1 PAGE

```c
typedef struct PAGE {
    unum PAGE_DIRECTORY_INDEX;
    unum PAGE_TABLE_INDEX;
    unum OFFSET;
} PAGE;
```

**Purpose:** Stores the three components extracted from a virtual address.

| Member | Meaning |
|---|---|
| PAGE_DIRECTORY_INDEX | Index used to select a Page Directory entry |
| PAGE_TABLE_INDEX | Index used to select a PTE |
| OFFSET | Byte position within the page |

**Why do we need it?**

Instead of maintaining three separate variables, we group the extracted address information into one structure.

The `page_details()` function returns this structure.

### 4.2 TLB

```c
typedef struct TLB {
    unum VIRTUAL_PAGE_NUMBER;
    unum PHYSICAL_FRAME_NUMBER;
    int ValidBit;
} TLB;
```

**Purpose:** Represents one entry in the Translation Lookaside Buffer.

| Member | Meaning |
|---|---|
| VIRTUAL_PAGE_NUMBER | Virtual page whose translation is cached |
| PHYSICAL_FRAME_NUMBER | Physical frame associated with that virtual page |
| ValidBit | Indicates whether the entry contains a usable translation |

**Why do we need it?**

The TLB avoids repeatedly accessing the Page Tables for recently used translations.

The simulator maintains an array of 16 such entries:

`TLB tlb[N];`

### 4.3 PAGE_TABLE_ENTRY (PTE)

```c
typedef struct PTE {
    unum PHYSICAL_FRAME_NUMBER;
    int PRESENT_BIT;
} PAGE_TABLE_ENTRY;
```

**Purpose:** Represents one entry inside a Page Table.

| Member | Meaning |
|---|---|
| PHYSICAL_FRAME_NUMBER | Physical frame mapped to the virtual page |
| PRESENT_BIT | Indicates whether the page is currently resident in RAM |

**Why do we need it?**

The PTE stores the actual mapping used by the Page Table.

The Present Bit helps distinguish between a page that is already in RAM and one that requires page fault handling.

### 4.4 PAGE_TABLE

```c
typedef struct PT {
    PAGE_TABLE_ENTRY TABLE[ENTRY_NUMBER];
} PAGE_TABLE;
```

**Purpose:** Represents the second level of the two-level paging structure.

Each Page Table contains 1024 Page Table Entries.

**Why do we need it?**

After extracting the Page Table Index, the simulator uses it to access the corresponding PTE.

For example:

`TABLE[PAGE_TABLE_INDEX]`

### 4.5 PAGE_DIRECTORY

```c
PAGE_TABLE* PAGE_DIRECTORY[ENTRY_NUMBER];
```

**Purpose:** Represents the first level of the two-level paging structure.

It contains 1024 pointers to Page Tables.

**Why pointers?**

Page Tables are allocated dynamically instead of allocating all possible Page Tables at program startup.

This saves memory when only a small portion of the virtual address space is accessed.

### 4.6 Physical Frame Tracking

```c
int PHYSICAL_FRAME_CHECK[ENTRY_NUMBER] = {0};
```

**Purpose:** Tracks the availability of physical frames.

| Value | Meaning |
|---|---|
| 0 | Frame is free |
| 1 | Frame is occupied |

**Why do we need it?**

When a page fault occurs, the simulator searches this array for an available physical frame.

Once a frame is allocated, its value changes to 1.

### 4.7 Other Global Variables

| Variable | Purpose |
|---|---|
| `TLB_HIT` | Counts successful TLB lookups |
| `TLB_MISS` | Counts unsuccessful TLB lookups |
| `PAGE_FAULT_COUNT` | Counts page faults |
| `REPLACEMENT_INDEX` | Tracks the next TLB entry to replace |

These variables maintain the simulator's state across multiple address requests.

---

## 5. Understanding Every Function

This section explains what each function does, why it exists and where it fits into the simulator.

### 5.1 get_string()

**Purpose:** Reads a dynamically sized string from standard input.

**How it works:**

1. Allocates an initial character buffer using `malloc()`.
2. Reads characters until a newline or EOF.
3. Uses `realloc()` when the buffer becomes full.
4. Adds the null terminator (`'\0'`).
5. Returns the allocated string.

**Why do we need it?**

It provides a way to read input without assuming a fixed maximum string length.

**Current status:** The function exists in the source code but is not currently used by `main()`, because the simulator accepts command-line arguments.

### 5.2 lower()

```c
unsigned char lower(char x);
```

**Purpose:** Converts a character into lowercase.

**How it works:**

It uses the standard C library function `tolower()`.

**Why do we need it?**

Hexadecimal addresses can contain uppercase or lowercase letters.

For example, `A` and `a` represent the same hexadecimal digit.

Converting characters to lowercase makes validation and conversion easier.

### 5.3 valid_address()

```c
int valid_address(string s);
```

**Purpose:** Checks whether the supplied virtual address is a valid hexadecimal string.

**Validation includes:**

- Rejecting NULL input.
- Rejecting empty input.
- Accepting an optional `0x` prefix.
- Checking that the address contains between 1 and 8 hexadecimal digits.
- Rejecting characters outside `0-9` and `A-F`.

**Return values:**

| Return Value | Meaning |
|---|---|
| 0 | Valid address |
| Non-zero | Invalid address |

**Why do we need it?**

The hexadecimal conversion function should only receive valid input.

This prevents invalid characters from producing incorrect address values.

### 5.4 hex_to_int()

```c
unum hex_to_int(string address);
```

**Purpose:** Converts a hexadecimal address into an unsigned 32-bit integer.

**How it works:**

It processes the hexadecimal digits from left to right.

For each digit:

`number = number * 16 + digit_value`

The function handles an optional `0x` prefix.

**Why do we need it?**

Bitwise operations require the address to be represented as an integer rather than a string.

### 5.5 page_details()

```c
PAGE page_details(unum address);
```

**Purpose:** Extracts the Page Directory Index, Page Table Index and Offset from a virtual address.

**How it works:**

It uses right shifts and bit masks to isolate the required bits.

- Bits 31–22: Page Directory Index.
- Bits 21–12: Page Table Index.
- Bits 11–0: Offset.

It returns a `PAGE` structure containing all three values.

**Why do we need it?**

It performs the address decomposition required for two-level paging.

### 5.6 initialize_TLB()

```c
void initialize_TLB(void);
```

**Purpose:** Initializes all TLB entries.

**How it works:**

It iterates through all 16 TLB entries and initializes:

- Physical Frame Number to 0.
- Virtual Page Number to 0.
- ValidBit to 0.

The replacement index is also initialized to 0.

**Why do we need it?**

At the beginning of a simulation, no translations should be cached.

This function is called once before processing the input addresses, not before every request.

### 5.7 lookup_TLB()

```c
unum lookup_TLB(unum VPN, int* not_found);
```

**Purpose:** Searches the TLB for a requested Virtual Page Number.

**How it works:**

1. Iterates through all 16 TLB entries.
2. Checks whether the ValidBit is set.
3. Compares the stored VPN with the requested VPN.
4. If a match is found, returns the corresponding PFN.
5. If no match is found, sets `not_found` to 1.

**Why do we need it?**

It determines whether the requested translation is already cached.

The pointer argument `not_found` allows the function to communicate whether the lookup was successful, while the return value carries the PFN.

### 5.8 get_PAGE_TABLE()

```c
PAGE_TABLE* get_PAGE_TABLE(unum PAGE_DIRECTORY_INDEX);
```

**Purpose:** Retrieves the Page Table corresponding to a Page Directory Index.

**How it works:**

1. Checks whether the corresponding Page Directory entry is NULL.
2. If it is NULL, dynamically allocates a Page Table using `calloc()`.
3. Stores the allocated pointer in the Page Directory.
4. Returns the Page Table pointer.

**Why do we need it?**

Page Tables are created only when required.

This avoids allocating all 1024 possible Page Tables at the beginning of the simulation.

### 5.9 insert_TLB()

```c
void insert_TLB(unum VPN, unum PFN);
```

**Purpose:** Inserts a successful VPN-to-PFN translation into the TLB.

**How it works:**

1. Searches for an invalid TLB entry.
2. If one is available, stores the VPN and PFN there.
3. Marks the entry as valid.
4. If all 16 entries are occupied, replaces an existing entry using the FIFO replacement policy.
5. Updates the replacement index using modulo arithmetic.

**Why do we need it?**

After a TLB miss is resolved, the translation is cached so that future requests for the same virtual page can be handled faster.

### 5.10 main()

```c
int main(int argc, string argv[]);
```

**Purpose:** Controls the complete address translation process.

**How it works:**

1. Checks whether virtual addresses were supplied through command-line arguments.
2. Initializes the TLB.
3. Iterates through the supplied addresses.
4. Validates each address.
5. Converts the hexadecimal address into an integer.
6. Extracts the PDI, PTI and offset.
7. Searches the TLB.
8. Handles TLB hits and misses.
9. Performs Page Table lookup when required.
10. Handles page faults and allocates physical frames.
11. Inserts resolved translations into the TLB.
12. Calculates the physical address.
13. Displays translation details and cumulative statistics.

**Why do we need it?**

It connects all the helper functions and data structures into one complete simulation.

---

## 6. Working Methodology

The simulator follows this sequence for every virtual address.

### Step 1: Input and Validation

The program receives virtual addresses as command-line arguments.

Example:

`./main 0x1234 0x1238 0x123C`

Each address is validated using `valid_address()`.

### Step 2: Hexadecimal Conversion

`hex_to_int()` converts the hexadecimal string into a 32-bit unsigned integer.

### Step 3: Address Decomposition

`page_details()` extracts the PDI, PTI and Offset.

The VPN is calculated separately using:

`VPN = ADDRESS >> 12`

### Step 4: TLB Lookup

`lookup_TLB()` searches for the VPN.

**On a TLB Hit:**

- Retrieve PFN from the TLB.
- Increment `TLB_HIT`.
- Skip Page Table lookup.

**On a TLB Miss:**

- Increment `TLB_MISS`.
- Retrieve the corresponding Page Table.
- Check the PTE Present Bit.

### Step 5: Page Table Lookup

If `PRESENT_BIT == 1`:

- Retrieve PFN from the PTE.

If `PRESENT_BIT == 0`:

- Increment the page fault counter.
- Find a free physical frame.
- Store the PFN in the PTE.
- Set the Present Bit to 1.

### Step 6: TLB Insertion

After successfully resolving a TLB miss, `insert_TLB()` stores the VPN-to-PFN mapping.

The TLB uses FIFO replacement.

### Step 7: Physical Address Calculation

The physical address is calculated using:

`PHYSICAL_ADDRESS = (PFN << 12) | OFFSET`

### Step 8: Statistics

The program updates and displays the cumulative hit count, miss count, page fault count, hit rate and miss rate.

---

## 7. TLB Replacement Policy

The simulator uses FIFO (First-In, First-Out) replacement for the TLB.

The TLB contains 16 entries.

Initially, invalid entries are filled. Once all entries are occupied, the oldest entry is replaced.

A circular replacement index is used:

`REPLACEMENT_INDEX = (REPLACEMENT_INDEX + 1) % N`

The modulo operation ensures that the index wraps back to 0 after reaching the last entry.

No priority queue or heap is required.

**Important:** TLB replacement and physical frame replacement are different mechanisms. This project implements FIFO replacement for the TLB but does not currently implement physical frame replacement.

---

## 8. Physical Frame Allocation

The simulator uses `PHYSICAL_FRAME_CHECK` to track free and occupied frames.

- 0 means free.
- 1 means occupied.

When a page fault occurs, the simulator searches for the first free frame.

The allocated frame is marked occupied and its PFN is stored in the corresponding PTE.

The current implementation does not replace existing physical pages when all frames are occupied. It reports memory exhaustion instead.

---

## 9. TLB Statistics

### TLB Hit Rate

`Hit Rate = (TLB Hits / Total TLB Lookups) × 100`

### TLB Miss Rate

`Miss Rate = (TLB Misses / Total TLB Lookups) × 100`

Where:

`Total TLB Lookups = TLB Hits + TLB Misses`

A page fault occurs only when the PTE Present Bit is 0.

Therefore, a TLB miss and a page fault are not the same event.

---

## 10. Effective Access Time (EAT)

Effective Access Time represents the average memory access time considering TLB performance.

For the simplified two-level Page Table model, assume:

- T = TLB lookup time.
- M = Main memory access time.
- h = TLB hit ratio.

On a TLB hit:

`T + M`

On a TLB miss without a page fault:

`T + 3M`

The three memory accesses are:

1. Page Directory access.
2. Page Table access.
3. Actual physical memory access.

Therefore:

`EAT = h(T + M) + (1 - h)(T + 3M)`

Page fault service time is excluded from this simplified formula.

**Implementation status:** Pending.

---

## 11. Running the Project

### Requirements

- GCC compiler.
- Linux environment or GitHub Codespaces.
- C compiler supporting the standard features used in the project.

### Compilation

```bash
gcc main.c -o main
```

### Execution

```bash
./main 0x1234 0x1238 0x123C 0x5234
```

Multiple virtual addresses can be supplied through command-line arguments.

The simulator maintains the same TLB, Page Directory, Page Tables and counters across all requests in one execution.

---

## 12. Example Execution

Input:

```bash
./main 0x1234 0x1238 0x123C 0x5234
```

Expected translation behaviour:

| Request | Address | VPN | TLB Status | Page Fault |
|---|---|---:|---|---|
| 1 | `0x1234` | 1 | Miss | Yes |
| 2 | `0x1238` | 1 | Hit | No |
| 3 | `0x123C` | 1 | Hit | No |
| 4 | `0x5234` | 5 | Miss | Yes |

The first three addresses belong to the same virtual page.

After the first request, the translation is cached in the TLB. The next two requests therefore produce TLB hits.

Expected cumulative statistics:

| Metric | Result |
|---|---:|
| TLB Hits | 2 |
| TLB Misses | 2 |
| Page Faults | 2 |
| TLB Hit Rate | 50% |
| TLB Miss Rate | 50% |

These results assume a fresh simulator execution.

---

## 13. Current Implementation Status

| Feature | Status |
|---|---|
| Virtual address validation | Implemented |
| Hexadecimal conversion | Implemented |
| Bitwise address decomposition | Implemented |
| Two-level Page Table | Implemented |
| Dynamic Page Table allocation | Implemented |
| TLB initialization | Implemented |
| TLB lookup | Implemented |
| TLB insertion | Implemented |
| FIFO TLB replacement | Implemented |
| Physical frame allocation | Implemented |
| Page fault detection | Implemented |
| Physical address calculation | Implemented |
| Multiple address processing | Implemented |
| TLB hit/miss rate calculation | Implemented |
| Effective Access Time calculation | Pending |
| Extensive testing and edge-case validation | Pending |

---

## 14. Limitations and Future Improvements

Potential improvements include:

1. Implementing Effective Access Time calculation.
2. Testing FIFO TLB replacement with more than 16 distinct virtual pages.
3. Implementing physical frame replacement algorithms.
4. Improving error handling and memory cleanup.
5. Adding additional address validation and page fault test cases.

---

## 15. Team Information

**Group 6 – Operating Systems Project**

- Pradyumna
- Swapnil Bose
- Mohd. Talha
- Arkaprava Jana

**Programming Language:** C

**Repository:** `OPERATING_SYSTEM_PROJECT`

---

## 16. Conclusion

This project demonstrates the fundamental concepts of virtual memory management and address translation.

By implementing a two-level Page Table and a TLB, the simulator illustrates how virtual addresses are translated into physical addresses, how caching improves translation efficiency, and how page faults are handled when pages are not resident in physical memory.

It also provides practical exposure to C programming concepts such as structures, pointers, arrays, dynamic memory allocation, functions and bitwise operations.