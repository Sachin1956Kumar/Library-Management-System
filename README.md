# Library Management System (C++)

A console-based Library Management System built in C++ to manage books, track issues/returns, and provide fast search — built primarily to apply core Data Structures (Hashing, Trie, Queue, Max-Heap) to a practical, real-world problem.

## Features

- **Add / Remove Books** — manage the library's book inventory.
- **Search by ID** — O(1) average-case lookup using a hash map (`unordered_map`).
- **Search by Title / Author (prefix)** — O(L) lookup using a custom **Trie**, where L is the length of the search query, instead of scanning every book.
- **Issue / Return Books** — tracks available copies and issue history.
- **Issue History** — maintained as a **Queue** (FIFO), showing the order books were borrowed.
- **Most Borrowed Books** — computed using a **Max-Heap** (`priority_queue`) to rank books by popularity.
- **Persistent Storage** — book data is saved to and loaded from a local file (`books.txt`), so data survives across runs.

## Why these data structures

| Operation | Data Structure | Complexity | Why |
|---|---|---|---|
| Lookup by Book ID | Hash Map | O(1) average | IDs are unique keys — direct hashing is fastest |
| Search by Title/Author prefix | Trie | O(L) | Search happens far more often than insertion; a Trie avoids scanning all n books for every query |
| Issue History | Queue | O(1) push/pop | History is naturally chronological (FIFO) |
| Most Borrowed Books | Max-Heap | O(log n) push, O(1) peek | Only need the top-k popular books, not a full sort |

A linear scan was the original approach for title/author search; it was replaced with a Trie since search is the dominant operation (run on every query) while insertion (adding a new book) is comparatively rare — so optimizing search at a small one-time insertion cost is the right tradeoff.

## How the Trie works here

- Every word in a book's title and author name is inserted into the Trie character by character (lowercased, alphanumeric only).
- Each Trie node stores the IDs of all books that have a word passing through that node — i.e., all books whose title/author contains a word with that prefix.
- A search for a prefix like `"har"` walks down the Trie character by character and returns every book ID stored at that node — so it matches *any* word in the title or author, not just the start of the full string (e.g., `"har"` matches both "**Har**ry Potter" and "Lord of the **Har**ringtons").
- The index is rebuilt on load and on book removal (simplest correct approach), and updated incrementally when a new book is added.

## Tech Stack

- **Language:** C++ (C++17)
- **Concepts used:** OOP, Hashing, Trie, Queue, Max-Heap (Priority Queue), File I/O, STL

## How to Run

```bash
g++ -std=c++17 LibraryManagementSystem.cpp -o lms
./lms
```

The program will create a `books.txt` file in the same directory to persist data between runs.

## Menu Options

```
1. Add Book
2. Remove Book
3. Search Book by ID
4. Search Book by Title (prefix)
5. Search Book by Author (prefix)
6. Display All Books
7. Issue Book
8. Return Book
9. Show Issue History
10. Show Most Borrowed Books
0. Exit
```

## Possible Future Improvements

- Add due dates and fine calculation for overdue books.
- Support multiple borrowers per book with reservation queues.
- Replace flat-file storage with a lightweight database (e.g., SQLite).
- Add unit tests for core operations (Trie search, issue/return logic).

## Author

`Sachin Kumar` — built as a practical application of core DSA concepts (Hashing, Trie, Queue, Heap) in C++.
