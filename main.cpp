#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <bitset>
#include <cmath>
#include <algorithm>
#include <cstdint>
using namespace std;

#define BITS_PER_BLOCK 64   // Each uint64_t can store 64 bits
#define NUM_COLS 42         // 42 columns in the kdd cup 99 dataset

typedef uint8_t row_t[NUM_COLS];
typedef vector<uint64_t> bitset_t;

// Helper function to compute the number of blocks needed for a given number of rows
inline size_t numBlocks(size_t numRows) {
    return (numRows + BITS_PER_BLOCK - 1) / BITS_PER_BLOCK;
}

// Function to set a bit in a bitset_t
inline void setBit(bitset_t& column, size_t row, bool value) {
    size_t blockIndex = row / BITS_PER_BLOCK;
    size_t bitIndex = row % BITS_PER_BLOCK;
    if (value) {
        column[blockIndex] |= (1ULL << bitIndex); // Set the bit
    } else {
        column[blockIndex] &= ~(1ULL << bitIndex); // Clear the bit
    }
}

// Function to get a bit from a bitset_t
inline bool getBit(const bitset_t& column, size_t row) {
    size_t blockIndex = row / BITS_PER_BLOCK;
    size_t bitIndex = row % BITS_PER_BLOCK;
    return (column[blockIndex] >> bitIndex) & 1;
}

// Function to compute the chi-square score
double chiSquare(const bitset_t& feature, const bitset_t& target, size_t numRows) {
    int tp = 0, tn = 0, fp = 0, fn = 0;

    for (size_t row = 0; row < numRows; ++row) {
        bool f = getBit(feature, row);
        bool t = getBit(target, row);

        if (f && t) ++tp;
        else if (!f && !t) ++tn;
        else if (f && !t) ++fp;
        else if (!f && t) ++fn;
    }

    double row1 = tp + fp, row2 = tn + fn;
    double col1 = tp + fn, col2 = fp + tn;

    double e1 = (row1 * col1) / numRows;
    double e2 = (row1 * col2) / numRows;
    double e3 = (row2 * col1) / numRows;
    double e4 = (row2 * col2) / numRows;

    if (e1 == 0 || e2 == 0 || e3 == 0 || e4 == 0) return 0.0;

    double chi2 = pow(tp - e1, 2) / e1 +
                  pow(fp - e2, 2) / e2 +
                  pow(fn - e3, 2) / e3 +
                  pow(tn - e4, 2) / e4;

    return chi2;
}

inline bool isInteger(const string &str) {
    if (str.empty()) return false;
    size_t startIdx = (str[0] == '-') ? 1 : 0;

    for (size_t i = startIdx; i < str.length(); ++i)
        if (!isdigit(str[i]))
            return false;
    return true;
}

void parseRow(const string &line, row_t &row) {
    stringstream ss(line);
    string token;
    int i = 0;
    while (getline(ss, token, ',') && i < NUM_COLS) {
        if (i == NUM_COLS - 1) {
            row[NUM_COLS - 1] = token == "normal" ? 0 : 1;
        } else {
            if (isInteger(token)) {
                int value = stoi(token);

                if (value == 0)
                    row[i] = 0;
                else if (value == 1)
                    row[i] = 1;
                else
                    row[i] = 2;
            } else {
                row[i] = 2;
            }
        }
        ++i;
    }
}

size_t parseCSVFile(
        const string& filePath,
        vector<bitset_t>& columns,
        vector<bool>& isBinary
) {
    ifstream file(filePath);
    if (!file.is_open()) {
        cerr << "Error: Could not open file." << endl;
        exit(1);
    }

    string line;
    size_t numRows = 0;

    if (getline(file, line)) {
        columns.resize(NUM_COLS, bitset_t(numBlocks(500000)));
        isBinary.resize(NUM_COLS, true);
    } else {
        cerr << "Error reading file header" << endl;
        return -1;
    }

    while (getline(file, line)) {
        row_t row;
        parseRow(line, row);

        for (size_t col = 0; col < NUM_COLS; ++col) {
            if (!isBinary[col]) continue;

            if (row[col] == 2) {
                isBinary[col] = false;
                continue;
            }
            setBit(columns[col], numRows, row[col]);
        }
        ++numRows;
    }

    file.close();
    return numRows;
}

int main() {
    const string filePath = "KDDCup99.csv";

    // Data storage
    vector<bitset_t> columns; // Bitsets for each column
    vector<bool> isBinary; // Track active columns

    // Parse CSV file
    size_t numRows = parseCSVFile(filePath, columns, isBinary);

    // Compute Chi-Square scores
    const auto& target = columns[NUM_COLS - 1];
    vector<pair<size_t, double>> scores; // Pair of (column index, chi-square score)

    for (size_t col = 0; col < NUM_COLS; ++col) {
        if (!isBinary[col]) continue;
        double score = chiSquare(columns[col], target, numRows);
        scores.emplace_back(col+1, score/numRows);
    }

    // Sort scores in descending order
    sort(scores.begin(), scores.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    // Output results
    ofstream output("ranked_columns.csv");
    output << "Column,Score\n";
    for (const auto& [col, score] : scores) {
        output << col << "," << score << "\n";
    }
    output.close();
    cout << "Results written to 'ranked_columns.csv'." << endl;
    return 0;
}