/*
Skeleton code for linear hash indexing
*/

#include <string>
#include <ios>
#include <fstream>
#include <vector>
#include <string>
#include <string.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include "classes.h"
using namespace std;

int mainMenu()
{
    int result;

    cout << "Enter (1) to lookup ID, (2) to exit \n";
    cin >> result;

    return result; // clean input
}

int lookupMenu()
{
    int result;
    cout << "Please enter the ID of the employee you want to look up \n";
    cin >> result;

    return result;
}

int main(int argc, char *const argv[])
{
    // Create the index
    LinearHashIndex emp_index("EmployeeIndex");
    emp_index.createFromFile("Employee.csv");

    bool flag = true;
    // Loop to lookup IDs until user is ready to quit
    // Employee is made up of id, name, bio, and manager-id.
    while (flag == true)
    {
        int option, id;

        // input handling
        option = mainMenu();
        if (option == 1)
        {
            id = lookupMenu();
            emp_index.findRecordById(id); // not good with switch statements
        }
        else if (option == 2)
        {
            flag = false;
            cout<< "program exit";
        }
        else
        {
            cout << "Please enter a valid input!\n";
            continue;
        }
    }

    return 0;
}
