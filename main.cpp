#include <iostream>
#include <fstream>      // For file I/O (ifstream, ofstream)
#include <sstream>      // For parsing strings (stringstream)
#include <string>
#include <vector>
#include <map>
#include <iomanip>      // For hex formatting (setw, setfill)
#include <cstdint>      // For uint32_t (32-bit unsigned integer)
#include <algorithm>    // For std::find_if
#include <set>          // Used for modifying I-format
#include <bitset>       // For generating debug string
using namespace std;

struct InstructionInfo {
    // Enum is nested inside the struct
    enum class Format { R, I, S, SB, U, UJ };

    std::string opcode;
    std::string funct3;
    std::string funct7;
    Format format; 
};

// --- Our "Instruction Brain" ---
std::map<std::string, InstructionInfo> instructionMap = {
    // R-Format
    { "add",   { "0110011", "000", "0000000", InstructionInfo::Format::R } },
    { "addw",  { "0111011", "000", "0000000", InstructionInfo::Format::R } },
    { "and",   { "0110011", "111", "0000000", InstructionInfo::Format::R } },
    { "or",    { "0110011", "110", "0000000", InstructionInfo::Format::R } },
    { "sll",   { "0110011", "001", "0000000", InstructionInfo::Format::R } },
    { "slt",   { "0110011", "010", "0000000", InstructionInfo::Format::R } },
    { "sra",   { "0110011", "101", "0100000", InstructionInfo::Format::R } },
    { "srl",   { "0110011", "101", "0000000", InstructionInfo::Format::R } },
    { "sub",   { "0110011", "000", "0100000", InstructionInfo::Format::R } },
    { "subw",  { "0111011", "000", "0100000", InstructionInfo::Format::R } },
    { "xor",   { "0110011", "100", "0000000", InstructionInfo::Format::R } },
    { "mul",   { "0110011", "000", "0000001", InstructionInfo::Format::R } }, // M Extension
    { "mulw",  { "0111011", "000", "0000001", InstructionInfo::Format::R } }, // M Extension
    { "div",   { "0110011", "100", "0000001", InstructionInfo::Format::R } }, // M Extension
    { "divw",  { "0111011", "100", "0000001", InstructionInfo::Format::R } }, // M Extension
    { "rem",   { "0110011", "110", "0000001", InstructionInfo::Format::R } }, // M Extension
    { "remw",  { "0111011", "110", "0000001", InstructionInfo::Format::R } }, // M Extension
    
    // I-Format
    { "addi",  { "0010011", "000", "NULL", InstructionInfo::Format::I } },
    { "addiw", { "0011011", "000", "NULL", InstructionInfo::Format::I } },
    { "andi",  { "0010011", "111", "NULL", InstructionInfo::Format::I } },
    { "ori",   { "0010011", "110", "NULL", InstructionInfo::Format::I } },
    { "lb",    { "0000011", "000", "NULL", InstructionInfo::Format::I } }, // Load
    { "ld",    { "0000011", "011", "NULL", InstructionInfo::Format::I } }, // Load
    { "lh",    { "0000011", "001", "NULL", InstructionInfo::Format::I } }, // Load
    { "lw",    { "0000011", "010", "NULL", InstructionInfo::Format::I } }, // Load
    { "jalr",  { "1100111", "000", "NULL", InstructionInfo::Format::I } }, // Load-like syntax

    // S-Format
    { "sb",    { "0100011", "000", "NULL", InstructionInfo::Format::S } },
    { "sw",    { "0100011", "010", "NULL", InstructionInfo::Format::S } },
    { "sh",    { "0100011", "001", "NULL", InstructionInfo::Format::S } },
    { "sd",    { "0100011", "011", "NULL", InstructionInfo::Format::S } },

    // SB-Format
    { "beq",   { "1100011", "000", "NULL", InstructionInfo::Format::SB } },
    { "bne",   { "1100011", "001", "NULL", InstructionInfo::Format::SB } },
    { "bge",   { "1100011", "101", "NULL", InstructionInfo::Format::SB } },
    { "blt",   { "1100011", "100", "NULL", InstructionInfo::Format::SB } },

    // U-Format
    { "auipc", { "0010111", "NULL", "NULL", InstructionInfo::Format::U } },
    { "lui",   { "0110111", "NULL", "NULL", InstructionInfo::Format::U } },

    // UJ-Format
    { "jal",   { "1101111", "NULL", "NULL", InstructionInfo::Format::UJ } }
};

//Symbol Table
map<string, long> symbolTable;

//HELPER FUNCTIONS

//remove leading spaces
string& ltrim(string& s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(), 
        [](unsigned char ch) {
        return !isspace(ch); //points to first non space char
        }) 
        p);//erases all whitespaces from beginning
    return s;
}

//remove trailing spaces
string& rtrim(std::string& s) {
    s.erase(find_if(s.rbegin(), s.rend(), //reverse iterators, traverse backward
        [](unsigned char ch) {
        return !isspace(ch);
    }).base(), //converts reverse to normal iterator, points to where trailig spaces begin
        s.end());
    return s;
}

string& trim(string& s) {
    return ltrim(rtrim(s));
}

string cleanLine(string line) {
    size_t commentPos = line.find('#'); //find comments
    if (commentPos !=string::npos) {
        line = line.substr(0, commentPos); //only take part befor comment starts
    }
    return trim(line);
}

//splits a line into meaningful tokens
vector<string> parseOperands(const string& line) {
    vector<string> tokens;
    string cleanLine = line;
    for (char& c : cleanLine) {
        //remove punctuation
        if (c == ',' || c == '(' || c == ')') {
            c = ' ';
        }
    }
    stringstream ss(cleanLine);
    string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

int registerToInt(const std::string& reg) {
    if (reg[0] == 'x') {
        return std::stoi(reg.substr(1));
    }
    // TODO: Add ABI names like "ra", "sp", "a0", etc.
    return 0; // Error or x0
}

long stringToLong(const std::string& s) {
    try {
        if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
            return std::stol(s, nullptr, 16);
        }
        return std::stol(s, nullptr, 10);
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: Invalid immediate value '" << s << "'" << std::endl;
        return 0;
    }
}

// Converts a 32-bit integer to a hex string.
std::string toHex(uint32_t value, bool pad = true) {
    std::stringstream ss;
    // Add std::uppercase here
    ss << "0x" << std::uppercase; 
    if (pad) {
       ss << std::setfill('0') << std::setw(8); 
    }
    ss << std::hex << value;
    return ss.str();
}

/**
 * Creates the compressed assembly string (e.g., "add x1,x2,x3")
 */
std::string getCompressedAssembly(const std::vector<std::string>& operands) {
    if (operands.empty()) return "";

    std::string instName = operands[0];
    const std::set<std::string> loadLike = {"lb", "ld", "lh", "lw", "jalr"};
    const std::set<std::string> storeLike = {"sb", "sw", "sh", "sd"};

    if (loadLike.count(instName)) {
        // Format: lw rd,imm(rs1)
        return instName + " " + operands[1] + "," + operands[2] + "(" + operands[3] + ")";
    }
    if (storeLike.count(instName)) {
        // Format: sw rs2,imm(rs1)
        return instName + " " + operands[1] + "," + operands[2] + "(" + operands[3] + ")";
    }
    if (instName == "jal" && operands.size() == 3) {
        // Format: jal rd,label
        return instName + " " + operands[1] + "," + operands[2];
    }

    // Default R-type, I-type (arith), SB-type
    // Format: add rd,rs1,rs2
    std::string result = instName + " " + operands[1];
    for (size_t i = 2; i < operands.size(); ++i) {
        result += "," + operands[i];
    }
    return result;
}

/**
 * Creates the debug string (e.g., "# 0110011-000-...")
 */
std::string getDebugString(const InstructionInfo& info, const std::vector<std::string>& operands, long offset = 0) {
    std::string opcode = info.opcode;
    std::string funct3 = info.funct3;
    std::string funct7 = info.funct7;
    std::string rd_s = "NULL", rs1_s = "NULL", rs2_s = "NULL", imm_s = "NULL";

    const std::set<std::string> loadLike = {"lb", "ld", "lh", "lw", "jalr"};

    try {
        if (info.format == InstructionInfo::Format::R) {
            rd_s = std::bitset<5>(registerToInt(operands[1])).to_string();
            rs1_s = std::bitset<5>(registerToInt(operands[2])).to_string();
            rs2_s = std::bitset<5>(registerToInt(operands[3])).to_string();
        } 
        else if (info.format == InstructionInfo::Format::I) {
            if (loadLike.count(operands[0])) { // lw rd, imm(rs1)
                rd_s = std::bitset<5>(registerToInt(operands[1])).to_string();
                rs1_s = std::bitset<5>(registerToInt(operands[3])).to_string();
                imm_s = std::bitset<12>(stringToLong(operands[2])).to_string();
            } else { // addi rd, rs1, imm
                rd_s = std::bitset<5>(registerToInt(operands[1])).to_string();
                rs1_s = std::bitset<5>(registerToInt(operands[2])).to_string();
                imm_s = std::bitset<12>(stringToLong(operands[3])).to_string();
            }
        }
        else if (info.format == InstructionInfo::Format::S) { // sw rs2, imm(rs1)
            rs1_s = std::bitset<5>(registerToInt(operands[3])).to_string();
            rs2_s = std::bitset<5>(registerToInt(operands[1])).to_string();
            imm_s = std::bitset<12>(stringToLong(operands[2])).to_string();
        }
        else if (info.format == InstructionInfo::Format::SB) { // beq rs1, rs2, label
            rs1_s = std::bitset<5>(registerToInt(operands[1])).to_string();
            rs2_s = std::bitset<5>(registerToInt(operands[2])).to_string();
            imm_s = std::bitset<13>(offset).to_string(); // Show 13-bit offset
        }
        else if (info.format == InstructionInfo::Format::U) { // lui rd, imm
            rd_s = std::bitset<5>(registerToInt(operands[1])).to_string();
            imm_s = std::bitset<20>(stringToLong(operands[2])).to_string(); // <-- FIX 2
        }
        else if (info.format == InstructionInfo::Format::UJ) { // jal rd, label
            rd_s = std::bitset<5>(registerToInt(operands[1])).to_string();
            imm_s = std::bitset<21>(offset).to_string(); // Show 21-bit offset
        }
    } catch (const std::exception& e) {
        // Handle error if parsing fails
        std::cerr << "Warning: Could not fully parse debug string for " << operands[0] << std::endl;
    }

    // Format: # opcode-funct3-funct7-rd-rs1-rs2-immediate
    return "# " + opcode + "-" + funct3 + "-" + funct7 + "-" + rd_s + "-" + rs1_s + "-" + rs2_s + "-" + imm_s;
}

// ==========================================================
//    ASSEMBLY FUNCTIONS (THE CORE LOGIC)
// ==========================================================

// R-Format
uint32_t assemble_R_format(const InstructionInfo& info, const std::vector<std::string>& operands) {
    uint32_t machineCode = 0;
    uint32_t rd  = registerToInt(operands[1]);
    uint32_t rs1 = registerToInt(operands[2]);
    uint32_t rs2 = registerToInt(operands[3]);
    
    uint32_t opcode = std::stoul(info.opcode, nullptr, 2);
    uint32_t funct3 = std::stoul(info.funct3, nullptr, 2);
    uint32_t funct7 = std::stoul(info.funct7, nullptr, 2);

    machineCode |= opcode;              // bits 0-6
    machineCode |= (rd  << 7);           // bits 7-11
    machineCode |= (funct3 << 12);      // bits 12-14
    machineCode |= (rs1 << 15);         // bits 15-19
    machineCode |= (rs2 << 20);         // bits 20-24
    machineCode |= (funct7 << 25);      // bits 25-31

    return machineCode;
}

// I-Format (Handles both arithmetic and load syntaxes)
uint32_t assemble_I_format(const InstructionInfo& info, const std::vector<std::string>& operands) {
    uint32_t machineCode = 0;
    uint32_t rd = 0, rs1 = 0;
    long imm = 0;

    const std::set<std::string> loadLike = {"lb", "ld", "lh", "lw", "jalr"};

    if (loadLike.count(operands[0])) { // lw rd, imm(rs1)
        rd  = registerToInt(operands[1]);
        imm = stringToLong(operands[2]);
        rs1 = registerToInt(operands[3]);
    } else { // addi rd, rs1, imm
        rd  = registerToInt(operands[1]);
        rs1 = registerToInt(operands[2]);
        imm = stringToLong(operands[3]);
    }
    
    uint32_t opcode = std::stoul(info.opcode, nullptr, 2);
    uint32_t funct3 = std::stoul(info.funct3, nullptr, 2);
    
    machineCode |= opcode;              // bits 0-6
    machineCode |= (rd  << 7);           // bits 7-11
    machineCode |= (funct3 << 12);      // bits 12-14
    machineCode |= (rs1 << 15);         // bits 15-19
    machineCode |= (imm << 20);         // bits 20-31 (imm[11:0])

    return machineCode;
}

// S-Format (Stores)
uint32_t assemble_S_format(const InstructionInfo& info, const std::vector<std::string>& operands) {
    uint32_t machineCode = 0;

    // S-Format: [imm[11:5] | rs2 | rs1 | funct3 | imm[4:0] | opcode]
    uint32_t rs2 = registerToInt(operands[1]); // sw rs2, imm(rs1)
    long imm     = stringToLong(operands[2]);
    uint32_t rs1 = registerToInt(operands[3]);

    uint32_t opcode = std::stoul(info.opcode, nullptr, 2);
    uint32_t funct3 = std::stoul(info.funct3, nullptr, 2);

    uint32_t imm_11_5 = (imm >> 5) & 0x7F; // imm[11:5]
    uint32_t imm_4_0  = imm & 0x1F;       // imm[4:0]
    
    machineCode |= opcode;              // bits 0-6
    machineCode |= (imm_4_0 << 7);      // bits 7-11 (imm[4:0])
    machineCode |= (funct3 << 12);      // bits 12-14
    machineCode |= (rs1 << 15);         // bits 15-19
    machineCode |= (rs2 << 20);         // bits 20-24
    machineCode |= (imm_11_5 << 25);    // bits 25-31 (imm[11:5])

    return machineCode;
}


// SB-Format (Branches)
uint32_t assemble_SB_format(const InstructionInfo& info, const std::vector<std::string>& operands, long currentAddress, const std::map<std::string, long>& symbolTable) {
    uint32_t machineCode = 0;

    uint32_t rs1 = registerToInt(operands[1]);
    uint32_t rs2 = registerToInt(operands[2]);
    
    std::string label = operands[3];
    if (symbolTable.find(label) == symbolTable.end()) {
        std::cerr << "Error: Undefined label '" << label << "'" << std::endl;
        return 0xDEADBEEF;
    }
    long labelAddress = symbolTable.at(label);
    long offset = labelAddress - currentAddress; 

    uint32_t imm_12 = (offset >> 12) & 1;    // imm[12]
    uint32_t imm_11 = (offset >> 11) & 1;    // imm[11]
    uint32_t imm_10_5 = (offset >> 5) & 0x3F; // imm[10:5]
    uint32_t imm_4_1 = (offset >> 1) & 0xF;   // imm[4:1]

    uint32_t opcode = std::stoul(info.opcode, nullptr, 2);
    uint32_t funct3 = std::stoul(info.funct3, nullptr, 2);

    machineCode |= opcode;              // bits 0-6
    machineCode |= (imm_11 << 7);       // bit 7 (imm[11])
    machineCode |= (imm_4_1 << 8);      // bits 8-11 (imm[4:1])
    machineCode |= (funct3 << 12);      // bits 12-14
    machineCode |= (rs1 << 15);         // bits 15-19
    machineCode |= (rs2 << 20);         // bits 20-24
    machineCode |= (imm_10_5 << 25);    // bits 25-30 (imm[10:5])
    machineCode |= (imm_12 << 31);      // bit 31 (imm[12])

    return machineCode;
}

// U-Format (lui, auipc)
uint32_t assemble_U_format(const InstructionInfo& info, const std::vector<std::string>& operands) {
    uint32_t machineCode = 0;

    uint32_t rd  = registerToInt(operands[1]);
    long imm     = stringToLong(operands[2]);

    uint32_t opcode = std::stoul(info.opcode, nullptr, 2);
    
    machineCode |= opcode;              // bits 0-6
    machineCode |= (rd << 7);           // bits 7-11
    machineCode |= (imm << 12);         // bits 12-31 (imm[31:12])
    
    return machineCode;
}


// UJ-Format (jal)
uint32_t assemble_UJ_format(const InstructionInfo& info, const std::vector<std::string>& operands, long currentAddress, const std::map<std::string, long>& symbolTable) {
    uint32_t machineCode = 0;

    uint32_t rd = registerToInt(operands[1]);
    
    std::string label = operands[2];
     if (symbolTable.find(label) == symbolTable.end()) {
        std::cerr << "Error: Undefined label '" << label << "'" << std::endl;
        return 0xDEADBEEF;
    }
    long labelAddress = symbolTable.at(label);
    long offset = labelAddress - currentAddress; 

    uint32_t imm_20 = (offset >> 20) & 1;     // imm[20]
    uint32_t imm_19_12 = (offset >> 12) & 0xFF; // imm[19:12]
    uint32_t imm_11 = (offset >> 11) & 1;     // imm[11]
    uint32_t imm_10_1 = (offset >> 1) & 0x3FF;  // imm[10:1]
    
    uint32_t opcode = std::stoul(info.opcode, nullptr, 2);

    machineCode |= opcode;              // bits 0-6
    machineCode |= (rd << 7);           // bits 7-11
    machineCode |= (imm_19_12 << 12);   // bits 12-19 (imm[19:12])
    machineCode |= (imm_11 << 20);      // bit 20 (imm[11])
    machineCode |= (imm_10_1 << 21);    // bits 21-30 (imm[10:1])
    machineCode |= (imm_20 << 31);      // bit 31 (imm[20])
    
    return machineCode;
}

// Stores the offset for branch/jump instructions
// so getDebugString can access it.
long lastOffset = 0; 

/**
 * Main assembler "switch" function.
 */
uint32_t assemble(const InstructionInfo& info, const std::vector<std::string>& operands, long currentAddress, const std::map<std::string, long>& symbolTable) {
    lastOffset = 0; // Reset offset
    switch (info.format) {
        case InstructionInfo::Format::R:
            return assemble_R_format(info, operands);
        case InstructionInfo::Format::I:
            return assemble_I_format(info, operands);
        case InstructionInfo::Format::S:
            return assemble_S_format(info, operands);
        case InstructionInfo::Format::SB:
        {
            std::string label = operands[3];
            if (symbolTable.count(label)) {
                lastOffset = symbolTable.at(label) - currentAddress;
            }
            return assemble_SB_format(info, operands, currentAddress, symbolTable);
        }
        case InstructionInfo::Format::U:
            return assemble_U_format(info, operands);
        case InstructionInfo::Format::UJ:
        {
            std::string label = operands[2];
            if (symbolTable.count(label)) {
                lastOffset = symbolTable.at(label) - currentAddress;
            }
            return assemble_UJ_format(info, operands, currentAddress, symbolTable);
        }
        default:
            std::cerr << "Error: Unknown instruction format for " << operands[0] << std::endl;
            return 0xDEADBEEF; // Error code
    }
}

// ==========================================================
//    MAIN FUNCTION (PASS 1 & 2)
// ==========================================================

int main() {
    std::string inputFilename = "input.asm";
    std::string outputFilename = "output.mc";

    // --- PASS 1: Build Symbol Table ---
    std::cout << "Starting Pass 1: Building Symbol Table..." << std::endl;
    std::ifstream inputFile_pass1(inputFilename);
    std::string line;

    long currentAddress = 0x00000000; // Code starts at 0x0
    long dataAddress = 0x10000000;    // Data starts at 0x10000000
    bool inTextSegment = true;        // Assume .text segment by default

    if (!inputFile_pass1.is_open()) {
        std::cerr << "Error: Could not open input file " << inputFilename << std::endl;
        return 1;
    }

    while (std::getline(inputFile_pass1, line)) {
        line = cleanLine(line);

        if (line == ".data") {
            inTextSegment = false;
            continue;
        }
        if (line == ".text") {
            inTextSegment = true;
            continue;
        }

        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string label = line.substr(0, colonPos);
            label = trim(label);
            symbolTable[label] = inTextSegment ? currentAddress : dataAddress;
            line = line.substr(colonPos + 1);
            line = trim(line);
        }

        if (line.empty()) continue; 

        if (inTextSegment) {
            currentAddress += 4;
        } else {
            // TODO: Handle .byte, .half, .word, .dword, .asciz
        }
    }
    inputFile_pass1.close();

    std::cout << "Pass 1 complete. Symbol Table:" << std::endl;
    for (const auto& [label, address] : symbolTable) {
        std::cout << "  " << label << ": " << toHex(address, false) << std::endl;
    }

    // --- PASS 2: Generate Machine Code ---
    std::cout << "Starting Pass 2: Generating Machine Code..." << std::endl;
    std::ifstream inputFile_pass2(inputFilename);
    std::ofstream outputFile(outputFilename);
    
    currentAddress = 0x00000000; // Reset for Pass 2
    inTextSegment = true;

    if (!inputFile_pass2.is_open()) {
        std::cerr << "Error: Could not open input file " << inputFilename << std::endl;
        return 1;
    }
    if (!outputFile.is_open()) {
        std::cerr << "Error: Could not open output file " << outputFilename << std::endl;
        return 1;
    }

    // Pre-read file to map addresses to original line text
    // (This block is corrected for the compiler error)
    std::map<long, std::string> originalLines;
    long tempAddr = 0x00000000;
    std::ifstream tempInputFile(inputFilename);
    while(std::getline(tempInputFile, line)) {
        std::string originalLine = line; 
        std::string cleaned = cleanLine(line);
        std::string afterColon; 
        bool isLabelOnly = false;

        size_t colonPos = cleaned.find(':');
        if (colonPos != std::string::npos) {
            
            afterColon = cleaned.substr(colonPos + 1); 
            if (trim(afterColon).empty()) { 
                isLabelOnly = true;
            }
            cleaned = afterColon;
        }
        
        if(!cleaned.empty() && cleaned[0] != '.' && !cleaned.empty()) {
            originalLines[tempAddr] = originalLine;
            tempAddr += 4;
        } else if (isLabelOnly) {
             originalLines[tempAddr] = originalLine;
        }
    }
    tempInputFile.close();


    // Now, the real Pass 2
    while (std::getline(inputFile_pass2, line)) {
        std::string originalFullLine = line; 
        std::string cleaned = cleanLine(line);

        if (cleaned == ".data") { inTextSegment = false; continue; }
        if (cleaned == ".text") { inTextSegment = true; continue; }

        if (cleaned.find(':') != std::string::npos) {
            cleaned = cleaned.substr(cleaned.find(':') + 1);
            cleaned = trim(cleaned);
        }

        if (cleaned.empty()) continue;

        if (inTextSegment) {
            std::vector<std::string> operands = parseOperands(cleaned);
            if (operands.empty()) continue;

            std::string instName = operands[0];
            if (instructionMap.count(instName)) {
                
                // 1. Assemble the machine code
                uint32_t machineCode = assemble(instructionMap[instName], operands, currentAddress, symbolTable);
                
                // 2. Get compressed assembly string
                std::string compressedAsm = getCompressedAssembly(operands);

                // 3. Get debug string (pass 'lastOffset' for branches/jumps)
                std::string debugString = getDebugString(instructionMap[instName], operands, lastOffset);

                // 4. Write the formatted line
                outputFile << toHex(currentAddress, false) << " " // false = don't pad address
                           << toHex(machineCode, true) << " , "   // true = pad machine code
                           << compressedAsm << " "
                           << debugString
                           << std::endl;
                
                currentAddress += 4;
            } else {
                std::cerr << "Warning: Skipping unknown instruction '" << instName << "'" << std::endl;
            }
        } else {
            // TODO: Handle data segment output
        }
    }
    
    // Add termination code
    outputFile << toHex(currentAddress, false) << " 0xFEEDC0DE" << " # End of text segment" << std::endl;

    inputFile_pass2.close();
    outputFile.close();

    std::cout << "Pass 2 complete. Output written to " << outputFilename << std::endl;

    return 0;
}
