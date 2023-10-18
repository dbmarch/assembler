// Assembler written by Cody March
/*
int main() 
openfile()      Open assembly file
filetoarray()   Read in assembly file, storing each non blank line as an index in an array of strings (or a vector would be great)
step1()         Break apart array of assembly strings into single IW instruction with gaps for double IW instructions
step2()         Translate array of machine code to binary, inserting addresses for jump to locations
arraytomif()    Print binary machine code array to mif file.

This represents the flow the program should go through
*/


#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>


using FileVector = std::vector<std::string>;

FileVector ParseFile (std::string &fileName);


// Array of strings containing all the Op Codes in their respective order. IN and OUT will become VADD and VSUB later.
std::string Ops[16] = {"LD", "ST", "CPY", "SWAP", "JMP", "ADD", "SUB", "ADDC", "SUBC", "NOT", "AND", "OR", "SRA", "RRC", "IN", "OUT"};


// Load , Store, Jump are doubles

// int j = 0;
// string jumps[16];

//*************************************************************************************************
int main(int argc, char** argv) {

    std::string fileName{"dxp_dcs.txt"};
    FileVector fileVector = ParseFile(fileName);

    for ( const auto line : fileVector) {
        std::cout << "!" << line  << "*" << std::endl;
    }

    return 0;
}


//*************************************************************************************************
FileVector ParseFile (std::string &fileName) {
    FileVector fileVector;
    std::ifstream myFile;


    myFile.open(fileName);
    
    if (myFile.is_open()) {
        // Step One: First pass through the loop
        // This loop will run until it reaches the end of the file
        while (!myFile.eof()) {
            std::string currentLine;
            std::getline(myFile, currentLine); // This will read the line from the file and store it into a designated string
            
            fileVector.push_back(currentLine);

    #if 0
            for (int i = 0; i < current_line.length(); i++) { // For loop to parse through the string.
                if (current_line.at(i) == ' ') {
                    // Do nothing - let the loop iterate onward since it is a space
                }
                else if (current_line.at(i) == ';') {
                    // We need to exit the for loop since we have reached a comment or end of line. There is no further useful information here to work with
                    i = current_line.length();
                }
                else {
                    // We have reached a non-commented line character
                    if (current_line.at(i) == '@') {
                        string word;
                        for (i = i; current_line.at(i) != ' '; i++) {
                            jumps[j].append(&current_line.at(i));
                        }
                        j++;
                    }
                }
            }
            cout << endl;
    #endif        
        }


       myFile.close(); // Closing the file after completion
    } else {
        std::cout << "File could not be opened: " << fileName << std::endl;
    }
    return fileVector;
}