/*
    Library Management System
    --------------------------

    Features:
    - Add / Remove / Search books by ID (O(1) average lookup via hashing - unordered_map)
    - Search books by Title / Author prefix using a Trie (O(L) lookup, L = prefix length)
    - Issue / Return books (tracked with a queue of issued records)
    - Track most borrowed books using a Max-Heap (priority_queue)
    - Persistent storage using file handling (books.txt)
    - Simple menu-driven console interface

    by:- Sachin Kumar
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <queue>
#include <vector>
#include <ctime>
#include <algorithm>

using namespace std;

// ---------------------------------------------------------
// Book structure
// ---------------------------------------------------------
struct Book {
    int id;
    string title;
    string author;
    int totalCopies;
    int availableCopies;
    int timesIssued; // used for "most borrowed" tracking
};

// ---------------------------------------------------------
// Issue record (for issued books queue / history)
// ---------------------------------------------------------
struct IssueRecord {
    int bookId;
    string borrowerName;
    string issueDate;
};

// ---------------------------------------------------------
// Comparator for Max-Heap based on timesIssued
// ---------------------------------------------------------
struct CompareByPopularity {
    bool operator()(const Book& a, const Book& b) {
        return a.timesIssued < b.timesIssued; // max-heap: higher timesIssued = higher priority
    }
};

// ---------------------------------------------------------
// Trie Node
// Each node represents one character. bookIds stores every
// book whose title/author contains a word passing through
// this node (i.e. having this prefix).
// ---------------------------------------------------------
struct TrieNode {
    unordered_map<char, TrieNode*> children;
    vector<int> bookIds; // ids of books having a word with this prefix

    ~TrieNode() {
        for (auto& pair : children) delete pair.second;
    }
};

// ---------------------------------------------------------
// Trie class
// Supports inserting words (with an associated bookId) and
// prefix search returning all bookIds matching that prefix.
// Used to replace the old O(n) linear scan for title/author
// search with O(L) prefix lookup, L = length of query.
// ---------------------------------------------------------
class Trie {
private:
    TrieNode* root;

    string normalize(const string& word) {
        string result;
        for (char c : word) {
            if (isalnum((unsigned char)c)) {
                result += (char)tolower((unsigned char)c);
            }
        }
        return result;
    }

public:
    Trie() {
        root = new TrieNode();
    }

    ~Trie() {
        delete root;
    }

    // Insert a single word, tagging this book's id along every
    // node on the path (so a prefix lookup finds it).
    void insertWord(const string& rawWord, int bookId) {
        string word = normalize(rawWord);
        if (word.empty()) return;

        TrieNode* node = root;
        for (char c : word) {
            if (node->children.find(c) == node->children.end()) {
                node->children[c] = new TrieNode();
            }
            node = node->children[c];
            node->bookIds.push_back(bookId);
        }
    }

    // Insert every word of a multi-word string (title or author),
    // so "Lord" or "Rings" both independently match
    // "The Lord of the Rings".
    void insertText(const string& text, int bookId) {
        stringstream ss(text);
        string word;
        while (ss >> word) {
            insertWord(word, bookId);
        }
    }

    // Return all bookIds whose title/author has a word starting
    // with the given prefix. O(L + k) where L = prefix length,
    // k = number of matches stored at that node.
    vector<int> searchByPrefix(const string& rawPrefix) {
        string prefix = normalize(rawPrefix);
        TrieNode* node = root;

        for (char c : prefix) {
            auto it = node->children.find(c);
            if (it == node->children.end()) {
                return {}; // no word with this prefix
            }
            node = it->second;
        }

        // de-duplicate, since a book can match the same prefix
        // through multiple words (e.g. title and author both)
        vector<int> ids = node->bookIds;
        sort(ids.begin(), ids.end());
        ids.erase(unique(ids.begin(), ids.end()), ids.end());
        return ids;
    }

    void clear() {
        delete root;
        root = new TrieNode();
    }
};

// ---------------------------------------------------------
// LibrarySystem class
// ---------------------------------------------------------
class LibrarySystem {
private:
    unordered_map<int, Book> books;        // bookId -> Book   (O(1) lookup)
    queue<IssueRecord> issueQueue;          // tracks issue history in order
    Trie searchIndex;                       // prefix search over title/author words
    const string DATA_FILE = "books.txt";

    // Rebuilds the Trie from scratch using current `books` map.
    // Called after load, and any time it's simpler to rebuild
    // than patch (kept simple/correct over micro-optimizing removes).
    void rebuildSearchIndex() {
        searchIndex.clear();
        for (auto& pair : books) {
            Book& b = pair.second;
            searchIndex.insertText(b.title, b.id);
            searchIndex.insertText(b.author, b.id);
        }
    }

    string getCurrentDate() {
        time_t now = time(0);
        tm* ltm = localtime(&now);
        ostringstream oss;
        oss << (1900 + ltm->tm_year) << "-" 
            << (1 + ltm->tm_mon) << "-" 
            << ltm->tm_mday;
        return oss.str();
    }

public:
    LibrarySystem() {
        loadFromFile();
    }

    ~LibrarySystem() {
        saveToFile();
    }

    // -------------------------------------------------
    // File Handling: Load data on startup
    // -------------------------------------------------
    void loadFromFile() {
        ifstream inFile(DATA_FILE);
        if (!inFile.is_open()) {
            cout << "[Info] No existing data file found. Starting fresh.\n";
            return;
        }

        string line;
        while (getline(inFile, line)) {
            if (line.empty()) continue;
            stringstream ss(line);
            string idStr, title, author, totalStr, availStr, issuedStr;

            getline(ss, idStr, '|');
            getline(ss, title, '|');
            getline(ss, author, '|');
            getline(ss, totalStr, '|');
            getline(ss, availStr, '|');
            getline(ss, issuedStr, '|');

            Book b;
            b.id = stoi(idStr);
            b.title = title;
            b.author = author;
            b.totalCopies = stoi(totalStr);
            b.availableCopies = stoi(availStr);
            b.timesIssued = stoi(issuedStr);

            books[b.id] = b;
        }
        inFile.close();
        rebuildSearchIndex();
        cout << "[Info] Loaded " << books.size() << " book(s) from storage.\n";
    }

    // -------------------------------------------------
    // File Handling: Save data on exit
    // -------------------------------------------------
    void saveToFile() {
        ofstream outFile(DATA_FILE);
        for (auto& pair : books) {
            Book& b = pair.second;
            outFile << b.id << "|" << b.title << "|" << b.author << "|"
                    << b.totalCopies << "|" << b.availableCopies << "|"
                    << b.timesIssued << "\n";
        }
        outFile.close();
    }

    // -------------------------------------------------
    // Add a new book
    // -------------------------------------------------
    void addBook() {
        Book b;
        cout << "\nEnter Book ID: ";
        cin >> b.id;

        if (books.find(b.id) != books.end()) {
            cout << "[Error] Book ID already exists.\n";
            return;
        }

        cout << "Enter Title: ";
        cin.ignore();
        getline(cin, b.title);
        cout << "Enter Author: ";
        getline(cin, b.author);
        cout << "Enter Number of Copies: ";
        cin >> b.totalCopies;

        b.availableCopies = b.totalCopies;
        b.timesIssued = 0;

        books[b.id] = b;
        searchIndex.insertText(b.title, b.id);
        searchIndex.insertText(b.author, b.id);
        cout << "[Success] Book added successfully.\n";
    }

    // -------------------------------------------------
    // Remove a book
    // -------------------------------------------------
    void removeBook() {
        int id;
        cout << "\nEnter Book ID to remove: ";
        cin >> id;

        if (books.find(id) == books.end()) {
            cout << "[Error] Book not found.\n";
            return;
        }

        books.erase(id);
        rebuildSearchIndex();
        cout << "[Success] Book removed.\n";
    }

    // -------------------------------------------------
    // Search a book by ID (O(1) average case - hashing)
    // -------------------------------------------------
    void searchBook() {
        int id;
        cout << "\nEnter Book ID to search: ";
        cin >> id;

        auto it = books.find(id);
        if (it == books.end()) {
            cout << "[Error] Book not found.\n";
            return;
        }

        displayBook(it->second);
    }

    // -------------------------------------------------
    // Search books by title word prefix (Trie - O(L) lookup,
    // L = length of typed prefix, instead of O(n) linear scan
    // over every book).
    // -------------------------------------------------
    void searchByTitle() {
        string prefix;
        cout << "\nEnter Title prefix to search (e.g. 'har' matches 'Harry...'): ";
        cin.ignore();
        getline(cin, prefix);

        vector<int> matchIds = searchIndex.searchByPrefix(prefix);
        if (matchIds.empty()) {
            cout << "[Info] No matching books found.\n";
            return;
        }

        for (int id : matchIds) {
            auto it = books.find(id);
            if (it != books.end()) displayBook(it->second);
        }
    }

    // -------------------------------------------------
    // Search books by author name word prefix (same Trie,
    // since author words were inserted into it too).
    // -------------------------------------------------
    void searchByAuthor() {
        string prefix;
        cout << "\nEnter Author prefix to search: ";
        cin.ignore();
        getline(cin, prefix);

        vector<int> matchIds = searchIndex.searchByPrefix(prefix);
        if (matchIds.empty()) {
            cout << "[Info] No matching books found.\n";
            return;
        }

        for (int id : matchIds) {
            auto it = books.find(id);
            if (it != books.end()) displayBook(it->second);
        }
    }

    void displayBook(const Book& b) {
        cout << "\n-----------------------------\n";
        cout << "ID: " << b.id << "\n";
        cout << "Title: " << b.title << "\n";
        cout << "Author: " << b.author << "\n";
        cout << "Total Copies: " << b.totalCopies << "\n";
        cout << "Available Copies: " << b.availableCopies << "\n";
        cout << "Times Issued: " << b.timesIssued << "\n";
        cout << "-----------------------------\n";
    }

    // -------------------------------------------------
    // Display all books
    // -------------------------------------------------
    void displayAllBooks() {
        if (books.empty()) {
            cout << "\n[Info] No books in the library.\n";
            return;
        }
        cout << "\n=== All Books ===\n";
        for (auto& pair : books) {
            displayBook(pair.second);
        }
    }

    // -------------------------------------------------
    // Issue a book
    // -------------------------------------------------
    void issueBook() {
        int id;
        cout << "\nEnter Book ID to issue: ";
        cin >> id;

        auto it = books.find(id);
        if (it == books.end()) {
            cout << "[Error] Book not found.\n";
            return;
        }

        if (it->second.availableCopies <= 0) {
            cout << "[Error] No copies available right now.\n";
            return;
        }

        string borrower;
        cout << "Enter Borrower Name: ";
        cin.ignore();
        getline(cin, borrower);

        it->second.availableCopies--;
        it->second.timesIssued++;

        IssueRecord record;
        record.bookId = id;
        record.borrowerName = borrower;
        record.issueDate = getCurrentDate();

        issueQueue.push(record);

        cout << "[Success] Book issued to " << borrower << " on " << record.issueDate << ".\n";
    }

    // -------------------------------------------------
    // Return a book
    // -------------------------------------------------
    void returnBook() {
        int id;
        cout << "\nEnter Book ID to return: ";
        cin >> id;

        auto it = books.find(id);
        if (it == books.end()) {
            cout << "[Error] Book not found.\n";
            return;
        }

        if (it->second.availableCopies >= it->second.totalCopies) {
            cout << "[Error] All copies are already in the library.\n";
            return;
        }

        it->second.availableCopies++;
        cout << "[Success] Book returned successfully.\n";
    }

    // -------------------------------------------------
    // Show issue history (FIFO order using queue)
    // -------------------------------------------------
    void showIssueHistory() {
        if (issueQueue.empty()) {
            cout << "\n[Info] No issue records yet.\n";
            return;
        }

        queue<IssueRecord> temp = issueQueue; // copy to preserve original
        cout << "\n=== Issue History (oldest first) ===\n";
        while (!temp.empty()) {
            IssueRecord r = temp.front();
            temp.pop();
            cout << "Book ID: " << r.bookId
                 << " | Borrower: " << r.borrowerName
                 << " | Date: " << r.issueDate << "\n";
        }
    }

    // -------------------------------------------------
    // Show most borrowed books using Max-Heap
    // -------------------------------------------------
    void showMostBorrowed() {
        if (books.empty()) {
            cout << "\n[Info] No books available.\n";
            return;
        }

        priority_queue<Book, vector<Book>, CompareByPopularity> maxHeap;
        for (auto& pair : books) {
            if (pair.second.timesIssued > 0)
                maxHeap.push(pair.second);
        }

        if (maxHeap.empty()) {
            cout << "\n[Info] No books have been issued yet.\n";
            return;
        }

        cout << "\n=== Most Borrowed Books (Top 5) ===\n";
        int count = 0;
        while (!maxHeap.empty() && count < 5) {
            Book b = maxHeap.top();
            maxHeap.pop();
            cout << count + 1 << ". " << b.title << " (Issued " << b.timesIssued << " times)\n";
            count++;
        }
    }
};

// ---------------------------------------------------------
// Menu-driven main function
// ---------------------------------------------------------
void showMenu() {
    cout << "\n========== LIBRARY MANAGEMENT SYSTEM ==========\n";
    cout << "1. Add Book\n";
    cout << "2. Remove Book\n";
    cout << "3. Search Book by ID\n";
    cout << "4. Search Book by Title (prefix)\n";
    cout << "5. Search Book by Author (prefix)\n";
    cout << "6. Display All Books\n";
    cout << "7. Issue Book\n";
    cout << "8. Return Book\n";
    cout << "9. Show Issue History\n";
    cout << "10. Show Most Borrowed Books\n";
    cout << "0. Exit\n";
    cout << "=================================================\n";
    cout << "Enter your choice: ";
}

int main() {
    LibrarySystem library;
    int choice;

    do {
        showMenu();
        cin >> choice;

        switch (choice) {
            case 1: library.addBook(); break;
            case 2: library.removeBook(); break;
            case 3: library.searchBook(); break;
            case 4: library.searchByTitle(); break;
            case 5: library.searchByAuthor(); break;
            case 6: library.displayAllBooks(); break;
            case 7: library.issueBook(); break;
            case 8: library.returnBook(); break;
            case 9: library.showIssueHistory(); break;
            case 10: library.showMostBorrowed(); break;
            case 0:
                cout << "\nSaving data and exiting... Goodbye!\n";
                break;
            default:
                cout << "[Error] Invalid choice. Try again.\n";
        }
    } while (choice != 0);

    return 0;
}
