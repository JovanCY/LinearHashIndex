#include <string>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <bitset>
#include <math.h>
#include <fcntl.h>
#include <fstream>
using namespace std;

string cleanString(string input)
{ // cleans string from padding and endline
    string result;

    result = input.erase(input.find("/"), string::npos);

    return result;
}

class Record
{
public:
    int id, manager_id;
    std::string bio, name;

    Record(vector<std::string> fields)
    {
        id = stoi(fields[0]);
        name = fields[1];
        bio = fields[2];
        manager_id = stoi(fields[3]);
    }

    Record()
    {
        id = -1;
        name = "default";
        bio = "default";
        manager_id = -1;
    }

    Record(int i)
    {
        id = i;
        name = "default";
        bio = "default";
        manager_id = -1;
    }

    void print()
    {
        cout << "\tID: " << id << "\n";
        cout << "\tNAME: " << name << "\n";
        cout << "\tBIO: " << bio << "\n";
        cout << "\tMANAGER_ID: " << manager_id << "\n";
    }

    string toString()
    {
        string output = "";
        output += to_string(id) += ",";
        output += name += ",";
        output += bio += ",";
        output += to_string(manager_id) += ";";
        return output;
    }
};

vector<string> split(const char *str, char c = ' ')
{ // from https://stackoverflow.com/questions/53849/how-do-i-tokenize-a-string-in-c
    vector<string> result;

    do
    {
        const char *begin = str;

        while (*str != c && *str)
            str++;

        result.push_back(string(begin, str));
    } while (0 != *str++);

    return result;
}

class LinearHashIndex
{

private:
    const int BLOCK_SIZE = 4096;

    vector<int> blockDirectory; // Map the least-significant-bits of h(id) to a bucket location in EmployeeIndex (e.g., the jth bucket)
                                // can scan to correct bucket using j*BLOCK_SIZE as offset (using seek function)
                                // can initialize to a size of 256 (assume that we will never have more than 256 regular (i.e., non-overflow) buckets)
    int n;                      // The number of indexes in blockDirectory currently being used
    int i;                      // The number of least-significant-bits of h(id) to check. Will need to increase i once n > 2^i
    int numRecords;             // Records currently in index. Used to test whether to increase n
    int nextFreeBlock;          // Next place to write a bucket. Should increment it by BLOCK_SIZE whenever a bucket is written to EmployeeIndex
    string fName;               // Name of index file

    // Insert new record into index
    void insertRecord(Record record)
    {
        int bucketNum;
        int remap = -1; // bucket that needs to be remapped
        numRecords++;

        // rehash check
        if ((float)numRecords > (float)n * 5 * 0.7)
        {
            n++;
            insertEmptyBucket();

            blockDirectory.push_back(nextFreeBlock);
            nextFreeBlock += BLOCK_SIZE;

            if (n > static_cast<int>(pow(2, i)))
            {
                i++;
                remap = 0; // if we have a new bit, e,g, 100, the previous i-1LSB is always 0
            }
            else
            {
                remap = (n - 1) % static_cast<int>(pow(2, i - 1));
            }
            rehash(blockDirectory[remap], blockDirectory[n - 1]);
        }

        // Add record to the index in the correct block, creating a overflow block if necessary
        // hash
        bucketNum = findBucket(record.id);
        vector<string> ar = grabBucket(bucketNum);

        int n = stoi(ar[0]);
        int overflow = stoi(ar[1]);

        if (n < 5)
        {
            string toWrite = "";
            toWrite += to_string(n + 1) += ";";
            toWrite += to_string(overflow) += ";";
            for (int i = 0; i < n; i++)
            {
                toWrite += ar[i + 2] += ";";
            }
            toWrite += record.toString(); // append new record to bucket
            writeToBucket(bucketNum, toWrite);
        }
        else if (overflow == -1)
        { // if overflow doesn't exist
            overflow = nextFreeBlock;
            nextFreeBlock += BLOCK_SIZE;
            insertEmptyBucket();

            // write to original block
            string toWrite = "";
            toWrite += to_string(n) += ";";
            toWrite += to_string(overflow) += ";";
            for (int i = 0; i < n; i++)
            {
                toWrite += ar[i + 2] += ";";
            }
            writeToBucket(bucketNum, toWrite);

            // write to overflow bucket
            vector<string> ov = grabBucket(overflow);

            n = stoi(ov[0]);
            string toWrite1 = "";
            toWrite1 += to_string(n + 1) += ";";
            toWrite1 += to_string(-1) += ";";
            for (int i = 0; i < n; i++)
            {
                toWrite1 += ov[i + 2] += ";";
            }
            toWrite1 += record.toString(); // append new record to bucket
            writeToBucket(overflow, toWrite1);
        }
        else
        { // if overflow exists
            // write to overflow bucket
            vector<string> ov = grabBucket(overflow);

            n = stoi(ov[0]);
            overflow = stoi(ov[1]);
            string toWrite1 = "";
            toWrite1 += to_string(n + 1) += ";";
            toWrite1 += to_string(overflow) += ";";
            for (int i = 0; i < n; i++)
            {
                toWrite1 += ov[i + 2] += ";";
            }
            toWrite1 += record.toString(); // append new record to bucket
            writeToBucket(overflow, toWrite1);
        }
    }

    void rehash(int bucket, int nBucket)
    {
        vector<string> ar = grabBucket(bucket);
        int n = stoi(ar[0]);
        int overflow = stoi(ar[1]);

        vector<Record> records = recordsFromVector(ar);

        int oldBucketCount = 0;
        vector<Record> oldBucket;
        int newBucketCount = 0;
        vector<Record> newBucket;

        for (int i = 0; i < n; i++)
        {
            int check = findBucket(records[i].id);
            if (check == bucket)
            {
                oldBucketCount++;
                oldBucket.push_back(records[i]);
            }
            else if (check == nBucket)
            {
                newBucketCount++;
                newBucket.push_back(records[i]);
            }
            else
            {
                cout << "ERROR UNINTENDED DATA IN REHASH2\n";
            }
        }

        // rehash overflow
        if (overflow > 0)
        {
            vector<string> overflowAr = grabBucket(overflow);
            n = stoi(overflowAr[0]);

            vector<Record> orecords = recordsFromVector(overflowAr);

            for (int i = 0; i < n; i++)
            {
                if (findBucket(orecords[i].id) == bucket)
                {
                    oldBucketCount++;
                    oldBucket.push_back(orecords[i]);
                }
                else if (findBucket(orecords[i].id) == nBucket)
                {
                    newBucketCount++;
                    newBucket.push_back(orecords[i]);
                }
                else
                {
                    cout << "ERROR UNINTENDED DATA IN REHASH1\n";
                }
            }
        }

        // build string
        string toWriteO = ""; // old bucket string

        if (oldBucketCount > 5)
        {
            toWriteO += to_string(5) += ";";
            toWriteO += to_string(overflow) += ";";
            for (int i = 0; i < oldBucketCount && i < 5; i++)
            {
                toWriteO += oldBucket[i].toString();
            }
            writeToBucket(bucket, toWriteO);

            string toWriteOO = ""; // string of original overflow bucket
            toWriteOO += to_string(oldBucketCount - 5) += ";";
            toWriteOO += "-1;";
            for (int i = 0; i < (oldBucketCount - 5) && i < 5; i++)
            {
                toWriteOO += oldBucket[i + 5].toString();
            }
            writeToBucket(overflow, toWriteOO);
        }
        else
        { // overflow should be cleared
            toWriteO += to_string(oldBucketCount) += ";";
            toWriteO += to_string(overflow) += ";";
            for (int i = 0; i < oldBucketCount && i < 5; i++)
            {
                toWriteO += oldBucket[i].toString();
            }
            writeToBucket(bucket, toWriteO);
            writeToBucket(overflow, "0;-1;");
        }

        string toWriteN = ""; // new bucket string
        toWriteN += to_string(newBucketCount % 6) += ";";
        if (newBucketCount > 5)
        {
            int overflowN = nextFreeBlock;
            nextFreeBlock += BLOCK_SIZE;
            insertEmptyBucket();
            toWriteN += to_string(overflowN) += ";";
            for (int i = 0; i < 5; i++)
            {
                toWriteN += newBucket[i].toString();
            }
            writeToBucket(nBucket, toWriteN);

            string toWriteNO = ""; // overflow of newBlock
            toWriteNO += to_string(newBucketCount - 5) += ";";
            toWriteNO += "-1;";
            for (int i = 0; i < (newBucketCount - 5); i++)
            {
                toWriteN += newBucket[i + 5].toString();
            }
            writeToBucket(overflowN, toWriteNO);
        }
        else
        {
            toWriteN += "-1;";
            for (int i = 0; i < newBucketCount; i++)
            {
                toWriteN += newBucket[i].toString();
            }
            writeToBucket(nBucket, toWriteN);
        }
    }

    string pad(string input)
    { // adds padding to string
        string result = input;

        while (result.size() < BLOCK_SIZE - 2)
        {
            result.append("/");
        }
        result.append("\n");

        return result;
    }

    void insertEmptyBucket()
    {
        fstream f;
        f.open(fName, ios::app);
        string block = "0;-1;";
        block = pad(block);
        f << block;
        f.close();
    }

    // returns record vectors from a vector string
    vector<Record> recordsFromVector(vector<string> line)
    {
        vector<Record> result;
        int n = stoi(line[0]);
        for (int i = 0; i < n; i++)
        {
            result.push_back(recordFromString(line[i + 2]));
        }
        return result;
    }

    Record recordFromString(string line)
    {
        vector<string> ar = split(line.data(), ',');
        Record record(ar);
        return record;
    }

    Record grabRecordFromCsv(fstream &empin)
    {
        // Employee is made up of id, name, bio, and manager_id.
        string line, word;
        vector<string> info;
        // string info[4];
        Record emp;
        // grab entire line
        if (getline(empin, line, '\n'))
        {
            // turn line into a stream
            stringstream s(line);
            // gets everything in stream up to comma
            getline(s, word, ',');
            info.push_back(word);
            getline(s, word, ',');
            info.push_back(word);
            getline(s, word, ',');
            info.push_back(word);
            getline(s, word, ';');
            info.push_back(word);
            Record goodEmp(info);
            return goodEmp;
        }
        else
        {
            return emp;
        }
    }

    // bucket is represented as vector string,
    vector<string> grabBucket(int ignored)
    {
        ifstream input;
        char temp[4097];
        input.open(fName);

        // int ignored = blockDirectory[bucket];
        input.seekg(ignored);

        input.read(temp, 4095);
        string line(temp);
        line = cleanString(line);

        vector<string> ar = split(line.data(), ';');

        return ar;
    }

    // adds padding to given line, writes it to indexFile to bucket indicated
    void writeToBucket(int bucket, string line)
    {
        fstream file;
        file.open(fName);
        file.seekg(bucket);
        // char *a = pad(line).data();
        // file.write(a, 4096);
        file << pad(line);
    }

    // find bucket number given id
    int findBucket(int id)
    {
        int result, hashed;
        hashed = id % static_cast<int>(pow(2, 8));
        result = hashed % static_cast<int>(pow(2, i));

        if (result >= n)
        {
            result = hashed % static_cast<int>(pow(2, i - 1));
        }

        return blockDirectory[result];
    }

public:
    LinearHashIndex(string indexFileName)
    {
        n = 4; // Start with 4 buckets in index
        i = 2; // Need 2 bits to address 4 buckets
        numRecords = 0;
        nextFreeBlock = 0;
        fName = indexFileName;

        // Create your EmployeeIndex file and write out the initial 4 buckets
        fstream indexFile;
        indexFile.open(fName, std::fstream::out);

        string block = "0;-1;";
        block = pad(block);

        for (int i = 0; i < n; i++)
        {
            indexFile << block; //.write(block.data(), 4096);
            blockDirectory.push_back(nextFreeBlock);
            nextFreeBlock += BLOCK_SIZE;
        }
        // make sure to account for the created buckets by incrementing nextFreeBlock appropriately
        indexFile.close();
    }

    // Read csv file and add records to the index
    void createFromFile(string csvFName)
    {
        fstream input;
        input.open(csvFName);

        Record toRead(1);
        while (!input.eof())
        {
            toRead = grabRecordFromCsv(input);
            if (input.eof())
                break;
            insertRecord(toRead);
        }
        input.close();
    }

    // Given an ID, find the relevant record and print it
    void findRecordById(int id)
    {
        Record result(-1);
        int bucket = findBucket(id);
        bool found = false;

        vector<string> line = grabBucket(bucket);
        vector<Record> records = recordsFromVector(line);

        int n = stoi(line[0]);
        int overflow = stoi(line[1]);

        
        // found, print, if not found, go to overflow if exists

        for (int i = 0; i<n&& 1<5; i++){
            if (records[i].id == id){
                result = records[i];
                found = true;
                break;
            }
        }
        
        if (n == 5 && overflow > 0){//if overflow exists
            vector<string> o = grabBucket(overflow);
            vector<Record> oRec = recordsFromVector(o);
            n = stoi(o[0]);
            for (int i = 0; i<n && i<5; i++){
                if (oRec[i].id == id){
                    result = oRec[i];
                    found = true;
                    break;
                }
            }
        }

        if (found == false)
        {
            cout << "The ID you specified was not found\n";
        }
        else
        {
            result.print();
        }
    }

};