# Virtual-to-Physical Address Translation Simulator

A C-based simulation of virtual-to-physical address translation using a **Two-Level Page Table** and a **Translation Lookaside Buffer (TLB)**.

## 1. Project Overview

### Problem Statement

Simulate address translation from virtual to physical addresses using a two-level page table and a TLB. Use bitwise operations to extract page numbers and offsets, and report TLB hit rate, page faults, and effective access time.

### Objective

The objective is to understand how an operating system translates virtual addresses into physical addresses.

The simulator demonstrates:

- Two-level paging, Page Directory and Page Tables.
- Translation Lookaside Buffer (TLB).
- Bitwise address decomposition.
- Physical frame allocation and page fault handling.
- TLB hit/miss tracking and FIFO TLB replacement.
- Effective Access Time (EAT) calculation.

This is a software simulation written in C. It does not implement actual hardware or modify the memory management of the host operating system.

---

## 2. Understanding the Core Concepts

### 2.1 Virtual Memory

Virtual memory is a memory-management technique that provides processes with their own virtual address spaces. A program generates virtual addresses when accessing memory, and these addresses must be translated into physical addresses to access RAM.

### 2.2 Paging

Paging divides virtual memory into fixed-sized blocks called **pages**. Physical memory is divided into equally sized blocks called **frames**. A virtual page can be mapped to an available physical frame.

In this project, both page size and frame size are 4 KB.

### 2.3 Two-Level Page Table

The two-level Page Table consists of:

**Page Directory (First Level)**
- Contains 1024 entries.
- Each entry points to a Page Table.
- Page Tables are allocated dynamically when needed.

**Page Table (Second Level)**
- Each Page Table contains 1024 Page Table Entries (PTEs).
- Each PTE stores a Physical Frame Number (PFN) and a Present Bit.

### 2.4 Translation Lookaside Buffer (TLB)

The TLB is a small, fast cache that stores recently used address translations:

`Virtual Page Number (VPN) → Physical Frame Number (PFN)`

- **TLB Hit:** The requested VPN is found in the TLB.
- **TLB Miss:** The requested VPN is not found in the TLB, so the Page Table must be checked.

A TLB miss does not necessarily mean a page fault. The page may already be present in physical memory even though its translation is not cached in the TLB.

### 2.5 Page Fault

A page fault occurs when the requested virtual page is not currently present in physical memory. In this simulation, the program searches for a free physical frame, updates the corresponding PTE, marks the page as present, and inserts the resolved translation into the TLB.

The simulator models page fault handling through frame allocation. It does not perform actual disk I/O or load page contents from secondary storage.

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
| TLB Replacement Policy | FIFO |

### 3.1 Virtual Address Breakdown

A 32-bit virtual address is divided into three components:

| Component | Bits | Purpose |
|---|---:|---|
| Page Directory Index (PDI) | 10 | Selects a Page Directory entry |
| Page Table Index (PTI) | 10 | Selects a Page Table entry |
| Offset | 12 | Identifies a byte within the page |

`32 = 10 + 10 + 12`

### 3.2 Bitwise Address Extraction

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

This section describes the main data structures used by the simulator.

### 4.1 PAGE

```c
typedef struct PAGE {
    unum PAGE_DIRECTORY_INDEX;
    unum PAGE_TABLE_INDEX;
    unum OFFSET;
} PAGE;
```

**Purpose:** Stores the three components extracted from a virtual address. The `page_details()` function returns this structure.

| Member | Meaning |
|---|---|
| PAGE_DIRECTORY_INDEX | Index used to select a Page Directory entry |
| PAGE_TABLE_INDEX | Index used to select a PTE |
| OFFSET | Byte position within the page |

### 4.2 TLB

```c
typedef struct TLB {
    unum VIRTUAL_PAGE_NUMBER;
    unum PHYSICAL_FRAME_NUMBER;
    int ValidBit;
} TLB;
```

**Purpose:** Represents one entry in the TLB.

| Member | Meaning |
|---|---|
| VIRTUAL_PAGE_NUMBER | Virtual page whose translation is cached |
| PHYSICAL_FRAME_NUMBER | Physical frame associated with that virtual page |
| ValidBit | Indicates whether the entry contains a usable translation |

The simulator maintains an array of 16 TLB entries: `TLB tlb[N];`

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
| PRESENT_BIT | Indicates whether the page is currently resident in simulated RAM |

### 4.4 PAGE_TABLE

```c
typedef struct PT {
    PAGE_TABLE_ENTRY TABLE[ENTRY_NUMBER];
} PAGE_TABLE;
```

Each Page Table contains 1024 Page Table Entries.

### 4.5 PAGE_DIRECTORY

```c
PAGE_TABLE* PAGE_DIRECTORY[ENTRY_NUMBER];
```

The Page Directory contains 1024 pointers to Page Tables. Page Tables are allocated dynamically instead of allocating all possible Page Tables at program startup.

### 4.6 Physical Frame Tracking

```c
int PHYSICAL_FRAME_CHECK[ENTRY_NUMBER] = {0};
```

| Value | Meaning |
|---|---|
| 0 | Frame is free |
| 1 | Frame is occupied |

When a page fault occurs, the simulator searches this array for an available physical frame.

### 4.7 Other State Variables

| Variable | Purpose |
|---|---|
| `TLB_HIT` | Counts successful TLB lookups |
| `TLB_MISS` | Counts unsuccessful TLB lookups |
| `PAGE_FAULT_COUNT` | Counts page faults |
| `REPLACEMENT_INDEX` | Tracks the next TLB entry to replace |

These variables maintain the simulator's state across multiple address requests in one execution.

---

## 5. Understanding the Functions

### 5.1 get_string()

**Purpose:** Reads a dynamically sized string from standard input. It allocates an initial character buffer, reads characters until a newline or EOF, expands the buffer when required, adds a null terminator, and returns the allocated string.

**Current status:** The function is present in the source but is not used by `main()` when addresses are supplied as command-line arguments.

### 5.2 lower()

**Purpose:** Converts a character into lowercase using the standard C library function `tolower()`. This helps handle uppercase and lowercase hexadecimal digits consistently.

### 5.3 valid_address()

**Purpose:** Checks whether the supplied virtual address is a valid hexadecimal string.

Validation includes:
- Rejecting NULL and empty input.
- Accepting an optional `0x` prefix.
- Checking that the address contains between 1 and 8 hexadecimal digits.
- Rejecting characters outside `0-9` and `A-F` (case-insensitive).

The function returns `0` for a valid address and a non-zero value for invalid input.

### 5.4 hex_to_int()

**Purpose:** Converts a hexadecimal address into an unsigned 32-bit integer. It processes hexadecimal digits from left to right:

`number = number * 16 + digit_value`

The function handles an optional `0x` prefix.

### 5.5 page_details()

**Purpose:** Extracts the Page Directory Index, Page Table Index and Offset from a virtual address using right shifts and bit masks. It returns the values in a `PAGE` structure.

### 5.6 initialize_TLB()

**Purpose:** Initializes all TLB entries by setting their VPN, PFN and ValidBit to their initial values and initializing the replacement index. This function is called once before processing the input addresses, not before every request.

### 5.7 lookup_TLB()

**Purpose:** Searches the TLB for a requested VPN. It checks the validity of each entry and compares its stored VPN with the requested VPN. If a match is found, it returns the corresponding PFN. Otherwise, it reports that the translation was not found.

### 5.8 get_PAGE_TABLE()

**Purpose:** Retrieves the Page Table corresponding to a Page Directory Index. If the corresponding Page Directory entry is NULL, the function dynamically allocates a Page Table using `calloc()`, stores its pointer in the Page Directory, and returns it.

### 5.9 insert_TLB()

**Purpose:** Inserts a resolved VPN-to-PFN translation into the TLB. The function first uses an invalid entry if one is available. When all 16 entries are occupied, it replaces an entry according to the FIFO replacement policy.

### 5.10 effective_access_time()

**Purpose:** Calculates Effective Access Time using the accumulated TLB hit, TLB miss and page fault statistics, together with the supplied timing parameters. It distinguishes between TLB hits, non-faulting TLB misses, and TLB misses that result in page faults. The formula and assumptions are explained in Section 10.

### 5.11 main()

**Purpose:** Controls the complete address translation process.

The main function:
1. Checks whether virtual addresses were supplied through command-line arguments.
2. Initializes the TLB.
3. Processes the supplied addresses.
4. Validates each address and converts it into an integer.
5. Extracts the PDI, PTI and offset.
6. Searches the TLB and handles hits and misses.
7. Performs Page Table lookup when required.
8. Handles page faults and allocates physical frames.
9. Inserts resolved translations into the TLB.
10. Calculates the physical address.
11. Updates and displays cumulative statistics and Effective Access Time.

---

## 6. Working Methodology

The simulator follows this sequence for every virtual address.

### Step 1: Input and Validation

The program receives virtual addresses as command-line arguments.

```bash
./main 0x1234 0x1238 0x123C
```

Each address is validated using `valid_address()`.

### Step 2: Hexadecimal Conversion

`hex_to_int()` converts the hexadecimal string into a 32-bit unsigned integer.

### Step 3: Address Decomposition

`page_details()` extracts the PDI, PTI and Offset. The VPN is calculated separately:

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

If `PRESENT_BIT == 1`, retrieve the PFN from the PTE.

If `PRESENT_BIT == 0`:
- Increment the page fault counter.
- Search for a free physical frame.
- Store the PFN in the PTE.
- Set the Present Bit to 1.

If no free frame is available, the simulator reports memory exhaustion.

### Step 6: TLB Insertion

After successfully resolving a TLB miss, `insert_TLB()` stores the VPN-to-PFN mapping.

### Step 7: Physical Address Calculation

`PHYSICAL_ADDRESS = (PFN << 12) | OFFSET`

### Step 8: Statistics and EAT

The program updates and displays cumulative TLB hits, TLB misses, page faults, hit rate, miss rate and Effective Access Time.

---

## 7. TLB Replacement Policy

The simulator uses **FIFO (First-In, First-Out)** replacement for the TLB.

The TLB contains 16 entries. Initially, invalid entries are filled. Once all entries are occupied, the oldest entry is replaced.

A circular replacement index is used:

`REPLACEMENT_INDEX = (REPLACEMENT_INDEX + 1) % N`

The modulo operation ensures that the index wraps back to 0 after reaching the last entry.

**Important:** TLB replacement and physical frame replacement are different mechanisms. This project implements FIFO replacement for the TLB but does not currently implement physical frame replacement.

---

## 8. Physical Frame Allocation

The simulator uses `PHYSICAL_FRAME_CHECK` to track free and occupied frames.

- `0` means free.
- `1` means occupied.

When a page fault occurs, the simulator searches for the first free frame. The allocated frame is marked occupied and its PFN is stored in the corresponding PTE.

The current implementation does not replace existing physical pages when all frames are occupied. It reports memory exhaustion instead.

---

## 9. TLB Statistics

### 9.1 TLB Hit Rate

`Hit Rate = (TLB Hits / Total TLB Lookups) × 100`

### 9.2 TLB Miss Rate

`Miss Rate = (TLB Misses / Total TLB Lookups) × 100`

Where:

`Total TLB Lookups = TLB Hits + TLB Misses`

A TLB miss and a page fault are not the same event. A TLB miss can be resolved by finding the requested page already present in the Page Table.

In this simulator, page faults occur after a TLB miss because the TLB is checked before the Page Table.

---

## 10. Effective Access Time (EAT)

**Effective Access Time (EAT)** represents the average time required to complete a memory access, taking the different address translation paths into account.

The simulator uses a three-path model.

### 10.1 Timing Parameters

| Symbol | Meaning |
|---|---|
| `T` | TLB lookup time |
| `M` | Main memory access time |
| `P` | Page fault service time |
| `H` | Number of TLB hits |
| `Q` | Number of non-faulting TLB misses |
| `F` | Number of page faults |
| `N` | Total number of address requests |

The number of non-faulting TLB misses is:

`Q = TLB_MISS - PAGE_FAULT_COUNT`

The total number of requests is:

`N = TLB_HIT + TLB_MISS`

### 10.2 Access Time for Each Path

**Case 1: TLB Hit**

The translation is found in the TLB, so the Page Table does not need to be accessed.

`Access Time = T + M`

**Case 2: TLB Miss Without a Page Fault**

The simulator must access the Page Directory, Page Table, and actual physical memory.

`Access Time = T + 3M`

**Case 3: TLB Miss With a Page Fault**

The model includes the page fault service time and the successful two-level translation and memory access path.

`Access Time = T + 3M + P`

### 10.3 EAT Formula

The weighted average is:

`EAT = [H(T + M) + Q(T + 3M) + F(T + 3M + P)] / N`

Substituting the counters:

`EAT = [TLB_HIT(T + M) + (TLB_MISS - PAGE_FAULT_COUNT)(T + 3M) + PAGE_FAULT_COUNT(T + 3M + P)] / (TLB_HIT + TLB_MISS)`

This model counts each request in exactly one of the three access paths. Page faults are included within TLB misses in this simulator, so they must not be counted twice.

### 10.4 Timing Units

The program accepts:
- TLB lookup time in nanoseconds.
- Main memory access time in nanoseconds.
- Page fault service time in milliseconds.

Page fault service time is converted into nanoseconds before the EAT calculation:

`1 millisecond = 1,000,000 nanoseconds`

For example:

`8 ms = 8,000,000 ns`

The final EAT is displayed in nanoseconds.

### 10.5 Example Timing Assumptions

The following values are illustrative inputs for the simulation, not measured hardware timings:

| Parameter | Example Value |
|---|---:|
| TLB lookup time | 10 ns |
| Main memory access time | 100 ns |
| Page fault service time | 8 ms |

The timing parameters can be changed to observe how different assumptions affect EAT.

**Implementation status:** Implemented.

---

## 11. Running the Project

### Requirements

- GCC compiler.
- Linux environment or GitHub Codespaces.
- C compiler supporting the standard features used in the project.

### Step 1: Compile

```bash
gcc main.c -o main
```

### Step 2: Execute

```bash
./main 0x1234 0x1238 0x123C 0x5234
```

Multiple virtual addresses can be supplied through command-line arguments. The simulator maintains the same TLB, Page Directory, Page Tables, physical frame allocation state and counters across all requests in one execution.

The timing parameters are supplied when prompted by the program.

---

## 12. Example Execution

### Input

```bash
./main 0x1234 0x1238 0x2341 0x123C
```

Assume the following timing parameters:
- TLB lookup time = 10 ns.
- Main memory access time = 100 ns.
- Page fault service time = 8 ms.

### Expected Translation Behaviour

| Request | Address | VPN | TLB Status | Page Fault |
|---|---|---:|---|---|
| 1 | `0x1234` | 1 | Miss | Yes |
| 2 | `0x1238` | 1 | Hit | No |
| 3 | `0x2341` | 2 | Miss | Yes |
| 4 | `0x123C` | 1 | Hit | No |

The first, second and fourth addresses belong to the same virtual page. After the first request, the translation is cached in the TLB. Subsequent requests for that virtual page can therefore produce TLB hits.

### Expected Final Statistics

| Metric | Result |
|---|---:|
| Total Requests | 4 |
| TLB Hits | 2 |
| TLB Misses | 2 |
| Page Faults | 2 |
| TLB Hit Rate | 50% |
| TLB Miss Rate | 50% |
| Effective Access Time | 4,000,210 ns |

The EAT calculation is:

- Two TLB hits: `2 × (10 + 100) = 220 ns`
- Two page faults: `2 × (10 + 3 × 100 + 8,000,000) = 16,000,620 ns`

Therefore:

`EAT = (220 + 16,000,620) / 4`

`EAT = 4,000,210 ns`

These results assume a fresh simulator execution, successful frame allocation, and the timing values shown above.

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
| Page fault detection and handling | Implemented |
| Physical address calculation | Implemented |
| Multiple address processing | Implemented |
| TLB hit/miss rate calculation | Implemented |
| Effective Access Time calculation | Implemented |
| Configurable timing parameters | Implemented |
| Extensive testing and edge-case validation | Further testing recommended |

---

## 14. Limitations and Future Improvements

Potential improvements include:

1. Testing FIFO TLB replacement with more than 16 distinct virtual pages.
2. Implementing physical frame replacement algorithms.
3. Improving memory cleanup for dynamically allocated Page Tables.
4. Expanding error handling and edge-case testing.
5. Adding more test cases for page faults and memory exhaustion.

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

The project also calculates TLB hit rate, miss rate and Effective Access Time using configurable timing assumptions. It provides practical exposure to C programming concepts such as structures, pointers, arrays, dynamic memory allocation, functions and bitwise operations.
