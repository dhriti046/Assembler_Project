#include <iostream>
#include <fstream>      // For file I/O (ifstream, ofstream)
#include <sstream>      // For parsing strings (stringstream)
#include <string>
#include <vector>
#include <map>
#include <iomanip>      // For hex formatting (setw, setfill)
#include <cstdint>      // For uint32_t (32-bit unsigned integer)
#include <algorithm>    // For find_if
#include <set>          // Used for modifying I-format
#include <bitset>       // For generating debug string
using namespace std;

struct InstructionInfo {
    enum class Format 
    { R, I, S, SB, U, UJ };
    string opcode;
    string funct3;
    string funct7;
    Format format; 
};

//instructuon set mapp
map<string, InstructionInfo> instructionMap = {
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

//symbil Table
map<string, long> symbolTable;

//remove leading spaces
string& ltrim(string& s) 
{
    s.erase(s.begin(), find_if(s.begin(), s.end(),[](unsigned char ch) 
        {
            return !isspace(ch); // points to first non-space char
        })
    );
    return s;
}

//remove trailing spaces
string& rtrim(string& s) 
{
    s.erase(find_if(s.rbegin(), s.rend(),[](unsigned char ch) //reverse iterator
    {
        return !isspace(ch);
    }).base(), //converts reverse to normal iterator, points to where trailig spaces begin
        s.end());
    return s;
}

string& trim(string& s) 
{
    return ltrim(rtrim(s));
}

string cleanLine(string line) 
{
    size_t commentPos = line.find('#'); //find comments
    if (commentPos !=string::npos) 
    {
        line = line.substr(0, commentPos); //only take part befor comment starts
    }
    return trim(line);
}

//splits a line into meaningful tokens
vector<string> parseOperands(const string& line) 
{
    vector<string> tokens;
    string cleanLine = line;
    for (char& c : cleanLine) 
    {
        //remove punctuation
        if (c == ',' || c == '(' || c == ')') 
        {
            c = ' ';
        }
    }
    stringstream ss(cleanLine);
    string token;
    while (ss >> token)
    {
        tokens.push_back(token);
    }
    return tokens;
}

int registerToInt(const string& reg) 
{
    static const unordered_map<string, int> regMap = {
        {"zero", 0}, {"ra", 1}, {"sp", 2}, {"gp", 3}, {"tp", 4},
        {"t0", 5}, {"t1", 6}, {"t2", 7},
        {"s0", 8}, {"fp", 8}, {"s1", 9},
        {"a0", 10}, {"a1", 11}, {"a2", 12}, {"a3", 13}, {"a4", 14}, {"a5", 15}, {"a6", 16}, {"a7", 17},
        {"s2", 18}, {"s3", 19}, {"s4", 20}, {"s5", 21}, {"s6", 22}, {"s7", 23}, {"s8", 24}, {"s9", 25},
        {"s10", 26}, {"s11", 27},
        {"t3", 28}, {"t4", 29}, {"t5", 30}, {"t6", 31}
    };

    if (reg.size() > 1 && reg[0] == 'x') 
    {
        // numeric register like x5
        return stoi(reg.substr(1));
    }

    auto it = regMap.find(reg);
    if (it != regMap.end()) 
    {
        return it->second;
    }

    // If not found, return 0 (default to x0)
    return 0;
}

//check if no. in hex or dec and convert to long
//stol is converting from hex/dec to dec long int
long stringToLong(const string& s) 
{
    if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0)
        return stol(s, nullptr, 16);
    else
        return stol(s, nullptr, 10);
}

// Converts 64-bit integer to a hex string with custom pading
string Hexa(uint64_t value, int num_chars = 0) { // Default to 0
    stringstream ss;
    ss << "0x" << uppercase;
    if (num_chars > 0) {
        ss << setfill('0') << setw(num_chars);
    }
    ss << hex << value;
    return ss.str();
}

//the vector type to back to assembly string
string getCompressedAssembly(vector<string>& operands) {
    if (operands.empty()) return "";
    
    string instName = operands[0];
    const set<string> loadLike = {"lb", "ld", "lh", "lw", "jalr"};
    const set<string> storeLike = {"sb", "sw", "sh", "sd"};

    if (loadLike.count(instName)) {
        //eg lw rd,imm(rs1)
        return instName + " " + operands[1] + "," + operands[2] + "(" + operands[3] + ")";
    }
    if (storeLike.count(instName)) {
        //eg sw rs2,imm(rs1)
        return instName + " " + operands[1] + "," + operands[2] + "(" + operands[3] + ")";
    }
    if (instName == "jal" && operands.size() == 3) {
        //eg jal rd, label
        return instName + " " + operands[1] + "," + operands[2];
    }

    // Default r-type,i type,sb-type
    //eg add rd,rs1,rs2
    string result = instName + " " + operands[1];
    for (size_t i = 2; i < operands.size(); ++i) 
    {
        result += "," + operands[i];
    }
    return result;
}

//to get the # string
string getDebugString(InstructionInfo& info, vector<string>& operands, long offset = 0) 
{
    string opcode = info.opcode;
    string funct3 = info.funct3;
    string funct7 = info.funct7;
    string rd_s = "NULL", rs1_s = "NULL", rs2_s = "NULL", imm_s = "NULL";
    const set<string> loadLike = {"lb", "ld", "lh", "lw", "jalr"};

    if (info.format == InstructionInfo::Format::R) 
    {
        rd_s = bitset<5>(registerToInt(operands[1])).to_string();
        rs1_s = bitset<5>(registerToInt(operands[2])).to_string();
        rs2_s = bitset<5>(registerToInt(operands[3])).to_string();
        //imm_s remains "NULL"
    } 
    else if (info.format == InstructionInfo::Format::I) 
    {
        if (loadLike.count(operands[0])) 
        { // lw rd, imm(rs1)
            rd_s = bitset<5>(registerToInt(operands[1])).to_string();
            rs1_s = bitset<5>(registerToInt(operands[3])).to_string();
            imm_s = bitset<12>(stringToLong(operands[2])).to_string();
        } 
        else { // addi rd, rs1, imm
            rd_s = bitset<5>(registerToInt(operands[1])).to_string();
            rs1_s = bitset<5>(registerToInt(operands[2])).to_string();
            imm_s = bitset<12>(stringToLong(operands[3])).to_string();
        }
        // funct7 remains "NULL"
    }
    else if (info.format == InstructionInfo::Format::S) 
    { // sw rs2, imm(rs1)
        rs1_s = bitset<5>(registerToInt(operands[3])).to_string();
        rs2_s = bitset<5>(registerToInt(operands[1])).to_string();
        imm_s = bitset<12>(stringToLong(operands[2])).to_string();
        // funct7 remains "NULL"
    }
    else if (info.format == InstructionInfo::Format::SB) 
    { // beq rs1, rs2, label
        rs1_s = bitset<5>(registerToInt(operands[1])).to_string();
        rs2_s = bitset<5>(registerToInt(operands[2])).to_string();
        imm_s = bitset<13>(offset).to_string();
        // funct7 remains "NULL"
    }
    else if (info.format == InstructionInfo::Format::U) 
    { // lui rd, imm
        rd_s = bitset<5>(registerToInt(operands[1])).to_string();
        imm_s = bitset<20>(stringToLong(operands[2])).to_string(); 
        // funct3 and funct7 remain "NULL"
    }
    else if (info.format == InstructionInfo::Format::UJ) 
    { // jal rd, label
        rd_s = bitset<5>(registerToInt(operands[1])).to_string();
        imm_s = bitset<21>(offset).to_string();
        //funct3 and funct7 remain NULL
    }

    if (info.format == InstructionInfo::Format::I || info.format == InstructionInfo::Format::U || info.format == InstructionInfo::Format::UJ) {
        //these don't include rs2
        //opcode-funct3-funct7-rd-rs1-immediate
        return "# " + opcode + "-" + funct3 + "-" + funct7 + "-" + rd_s + "-" + rs1_s + "-" + imm_s;
    } 
    else 
    {
        // R, S, and SB use all 7 fields
        //opcode-funct3-funct7-rd-rs1-rs2-immediate
        return "# " + opcode + "-" + funct3 + "-" + funct7 + "-" + rd_s + "-" + rs1_s + "-" + rs2_s + "-" + imm_s;
    }
}
/*
//create the debug string
//printing 7 fields for all types
string getDebugString(InstructionInfo& info, vector<string>& operands, long offset = 0) 
{
    string opcode = info.opcode;
    string funct3 = info.funct3;
    string funct7 = info.funct7;
    string rd_s = "NULL", rs1_s = "NULL", rs2_s = "NULL", imm_s = "NULL";
    const set<string> loadLike = {"lb", "ld", "lh", "lw", "jalr"};

    if (info.format == InstructionInfo::Format::R) {
        rd_s = bitset<5>(registerToInt(operands[1])).to_string();
        rs1_s = bitset<5>(registerToInt(operands[2])).to_string();
        rs2_s = bitset<5>(registerToInt(operands[3])).to_string();
    } 
    else if (info.format == InstructionInfo::Format::I) 
    {
        if (loadLike.count(operands[0])) 
        { // lw rd, imm(rs1)
            rd_s = bitset<5>(registerToInt(operands[1])).to_string();
            rs1_s = bitset<5>(registerToInt(operands[3])).to_string();
            imm_s = bitset<12>(stringToLong(operands[2])).to_string();
        } 
        else 
        { // addi rd, rs1, imm
            rd_s = bitset<5>(registerToInt(operands[1])).to_string();
            rs1_s = bitset<5>(registerToInt(operands[2])).to_string();
            imm_s = bitset<12>(stringToLong(operands[3])).to_string();
        }
    }
    else if (info.format == InstructionInfo::Format::S) 
    { // sw rs2, imm(rs1)
        rs1_s = bitset<5>(registerToInt(operands[3])).to_string();
        rs2_s = bitset<5>(registerToInt(operands[1])).to_string();
        imm_s = bitset<12>(stringToLong(operands[2])).to_string();
    }
    else if (info.format == InstructionInfo::Format::SB) 
    { // beq rs1, rs2, label
        rs1_s = bitset<5>(registerToInt(operands[1])).to_string();
        rs2_s = bitset<5>(registerToInt(operands[2])).to_string();
        imm_s = bitset<13>(offset).to_string(); //13-bit offset
    }
    else if (info.format == InstructionInfo::Format::U) 
    { // lui rd, imm
        rd_s = bitset<5>(registerToInt(operands[1])).to_string();
        imm_s = bitset<20>(stringToLong(operands[2])).to_string(); 
    }
    else if (info.format == InstructionInfo::Format::UJ) 
    { // jal rd, label
        rd_s = bitset<5>(registerToInt(operands[1])).to_string();
        imm_s = bitset<21>(offset).to_string(); //21-bit offset
    }

    //opcode-func3-func7-rd-rs1-rs2-imm
    return "# " + opcode + "-" + funct3 + "-" + funct7 + "-" + rd_s + "-" + rs1_s + "-" + rs2_s + "-" + imm_s;
}
*/
//build machine code for r-format

uint32_t assemble_R_format(const InstructionInfo& info, const vector<string>& operands) {
    uint32_t machineCode = 0;
    //convert operands to register no.
    uint32_t rd  = registerToInt(operands[1]);
    uint32_t rs1 = registerToInt(operands[2]);
    uint32_t rs2 = registerToInt(operands[3]);
    
   //Convert opcode/funct3/funct7 to integer
   //so that we can use shift operations to generate machine code
    uint32_t opcode = stoul(info.opcode, nullptr, 2);
    uint32_t funct3 = stoul(info.funct3, nullptr, 2);
    uint32_t funct7 = stoul(info.funct7, nullptr, 2);

    machineCode |= opcode;//0-6
    machineCode |= (rd  << 7);//7-11
    machineCode |= (funct3 << 12);//12-14
    machineCode |= (rs1 << 15);//15-19
    machineCode |= (rs2 << 20);//20-24
    machineCode |= (funct7 << 25);//25-31
    return machineCode;
}

//i-foormat
uint32_t assemble_I_format(const InstructionInfo& info,const vector<string>& operands) {
    uint32_t machineCode = 0;
    uint32_t rd = 0, rs1 = 0;
    long imm = 0;
    const set<string> loadLike = {"lb", "ld", "lh", "lw", "jalr"};
    if (loadLike.count(operands[0])) 
    { 
        //lw rd, imm(rs1)
        rd  = registerToInt(operands[1]);
        imm = stringToLong(operands[2]);
        rs1 = registerToInt(operands[3]);
    } 
    else 
    { 
        //addi rd, rs1, imm
        rd  = registerToInt(operands[1]);
        rs1 = registerToInt(operands[2]);
        imm = stringToLong(operands[3]);
    }
    
    uint32_t opcode = stoul(info.opcode, nullptr, 2);
    uint32_t funct3 = stoul(info.funct3, nullptr, 2);
    
    machineCode |= opcode;//0-6
    machineCode |= (rd  << 7);//7-11
    machineCode |= (funct3 << 12);//12-14
    machineCode |= (rs1 << 15);//15-19
    machineCode |= (imm << 20);//20-31(imm[11:0])
    return machineCode;
}

// S-Format
uint32_t assemble_S_format(const InstructionInfo& info,const vector<string>& operands) 
{
    uint32_t machineCode = 0;
    //[imm[11:5],rs2,rs1,funct3,imm[4:0],opcode]
    uint32_t rs2 = registerToInt(operands[1]); // sw rs2, imm(rs1)
    long imm= stringToLong(operands[2]);
    uint32_t rs1 = registerToInt(operands[3]);

    uint32_t opcode = stoul(info.opcode, nullptr, 2);
    uint32_t funct3 = stoul(info.funct3, nullptr, 2);

    uint32_t imm_11_5 = (imm >> 5) & 0x7F;//imm[11:5]
    uint32_t imm_4_0  = imm & 0x1F;//imm[4:0]
    
    machineCode |= opcode;//0-6
    machineCode |= (imm_4_0 << 7);//7-11 (imm[4:0])
    machineCode |= (funct3 << 12);//12-14
    machineCode |= (rs1 << 15);//15-19
    machineCode |= (rs2 << 20);//20-24
    machineCode |= (imm_11_5 << 25);//25-31 (imm[11:5])

    return machineCode;
}


// SB-Format
uint32_t assemble_SB_format(const InstructionInfo& info,const vector<string>& operands, long currentAddress, const map<string, long>& symbolTable) 
{
    uint32_t machineCode = 0;

    uint32_t rs1 = registerToInt(operands[1]);
    uint32_t rs2 = registerToInt(operands[2]);
    
    string label = operands[3];
    if (symbolTable.find(label) == symbolTable.end()) 
    {
        cerr << "Error: Undefined label '" << label << "'" << endl;
        return 0xDEADBEEF;
    }
    long labelAddress = symbolTable.at(label);
    long offset = labelAddress - currentAddress; 

    uint32_t imm_12 = (offset >> 12) & 1;// imm[12]
    uint32_t imm_11 = (offset >> 11) & 1;// imm[11]
    uint32_t imm_10_5 = (offset >> 5) & 0x3F;// imm[10:5]
    uint32_t imm_4_1 = (offset >> 1) & 0xF;// imm[4:1]

    uint32_t opcode = stoul(info.opcode, nullptr, 2);
    uint32_t funct3 = stoul(info.funct3, nullptr, 2);

    machineCode |= opcode;//0-6
    machineCode |= (imm_11 << 7);//7 (imm[11])
    machineCode |= (imm_4_1 << 8);//8-11 (imm[4:1])
    machineCode |= (funct3 << 12);//12-14
    machineCode |= (rs1 << 15);//15-19
    machineCode |= (rs2 << 20);//20-24
    machineCode |= (imm_10_5 << 25);//25-30 (imm[10:5])
    machineCode |= (imm_12 << 31);//31 (imm[12])

    return machineCode;
}

//u-format(lui, auipc)
uint32_t assemble_U_format(const InstructionInfo& info, const vector<string>& operands) {
    uint32_t machineCode = 0;

    uint32_t rd  = registerToInt(operands[1]);
    long imm= stringToLong(operands[2]);
    uint32_t opcode = stoul(info.opcode, nullptr, 2);
    
    machineCode |= opcode;//0-6
    machineCode |= (rd << 7);//7-11
    machineCode |= (imm << 12);//12-31 (imm[31:12])
    
    return machineCode;
}

//UJ-Format (jal)
uint32_t assemble_UJ_format(const InstructionInfo& info, const vector<string>& operands, long currentAddress, const map<string, long>& symbolTable) 
{
    uint32_t machineCode = 0;
    uint32_t rd = registerToInt(operands[1]);
    string label = operands[2];
     if (symbolTable.find(label) == symbolTable.end()) 
     {
        cerr << "Error: Undefined label '" << label << "'" << endl;
        return 0xDEADBEEF;
    }
    long labelAddress = symbolTable.at(label);
    long offset = labelAddress - currentAddress; 

    uint32_t imm_20 = (offset >> 20) & 1;//imm[20]
    uint32_t imm_19_12 = (offset >> 12) & 0xFF;//imm[19:12]
    uint32_t imm_11 = (offset >> 11) & 1;     // imm[11]
    uint32_t imm_10_1 = (offset >> 1) & 0x3FF;  // imm[10:1]
    
    uint32_t opcode = stoul(info.opcode, nullptr, 2);

    machineCode |= opcode;//0-6
    machineCode |= (rd << 7);//7-11
    machineCode |= (imm_19_12 << 12);//12-19 (imm[19:12])
    machineCode |= (imm_11 << 20);//20 (imm[11])
    machineCode |= (imm_10_1 << 21);//21-30 (imm[10:1])
    machineCode |= (imm_20 << 31);//31 (imm[20])
    
    return machineCode;
}

//to stores the offset for branch/jump inst
long lastOffset = 0; 
uint32_t assemble(const InstructionInfo& info, const vector<string>& operands, long currentAddress, const map<string, long>& symbolTable) 
{
   
   //lastOFfset stores the branch/jump target offset 
   lastOffset = 0; //reset offset
    switch (info.format) 
    {
        case InstructionInfo::Format::R:
            return assemble_R_format(info, operands);
        case InstructionInfo::Format::I:
            return assemble_I_format(info, operands);
        case InstructionInfo::Format::S:
            return assemble_S_format(info, operands);
        case InstructionInfo::Format::SB:
        {
            string label = operands[3];
            if (symbolTable.count(label))
            {
                lastOffset = symbolTable.at(label) - currentAddress;
            }
            return assemble_SB_format(info, operands, currentAddress, symbolTable);
        }
        case InstructionInfo::Format::U:
            return assemble_U_format(info, operands);
        case InstructionInfo::Format::UJ:
        {
            string label = operands[2];
            if (symbolTable.count(label)) 
            {
                lastOffset = symbolTable.at(label) - currentAddress;
            }
            return assemble_UJ_format(info, operands, currentAddress, symbolTable);
        }
        default:
            cerr << "Error:Unknown instruction format for " << operands[0] << endl;
            return 0xDEADBEEF; // Error
    }
}

int main() 
{
    string inputFilename = "input.asm";
    string outputFilename = "output.mc";

    //build symbol table
    cout << "Starting Pass 1: Building Symbol Table..." << endl;
    ifstream inputFile_pass1(inputFilename);
    string line;
    long currentAddress = 0x00000000;
    long dataAddress = 0x10000000;
    bool inTextSegment = true;

    if (!inputFile_pass1.is_open()) 
    {
        cerr << "Error:Could not open input file " << inputFilename << endl;
        return 1;
    }

    while (getline(inputFile_pass1, line)) 
    {
        line = cleanLine(line);
        if (line == ".data") 
        { inTextSegment = false; 
            continue; }
        if (line == ".text") 
        { inTextSegment = true; 
            continue; }
        size_t colon = line.find(':');
        //label found or not
        //if found,put it in symbol table with current address/data address
        if (colon != string::npos) 
        {
            string label = line.substr(0, colon);
            label = trim(label);
            symbolTable[label] = inTextSegment ? currentAddress: dataAddress;
            line = line.substr(colon + 1);
            line = trim(line);
        }
        if (line.empty()) continue; 
        if (inTextSegment) 
        {
            currentAddress += 4;
        } 
        //if inside data segment
        else 
        {
            vector<string> operands = parseOperands(line);
            if (operands.empty()) continue;
            string directive = operands[0];
            //update data address based on directive
            if (directive == ".byte") dataAddress += 1;
            else if (directive == ".half") dataAddress += 2;
            else if (directive == ".word") dataAddress += 4;
            else if (directive == ".dword") dataAddress += 8;
            else if (directive == ".asciz") 
            {
                size_t firstQuote = line.find('\"');
                size_t lastQuote = line.rfind('\"');
                if (firstQuote != string::npos && lastQuote != string::npos && firstQuote != lastQuote) {
                    string strData = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                    dataAddress += strData.length() + 1;
                }
            }
        }
    }
    inputFile_pass1.close();
    cout << "Pass 1 complete. Symbol Table:" << endl;
    for (const auto& [label, address]: symbolTable) {
        cout << "  " << label << ": " << Hexa(address, 0) << endl;
    }

    //generate machine Code
    cout << "Starting Pass 2: Generating Machine Code..." << endl;
    ifstream inputFile_pass2(inputFilename);
    ofstream outputFile(outputFilename);
    
    if (!inputFile_pass2.is_open() || !outputFile.is_open()) {
        cerr << "Error:cant open input/output files for Pass 2" << endl;
        return 1;
    }

    currentAddress = 0x00000000; //reset text address
    inTextSegment = true;//start by assuming we are in .text

    while (getline(inputFile_pass2, line)) {
        string cleaned = cleanLine(line);
        if (cleaned == ".data") { inTextSegment = false; continue; }
        if (cleaned == ".text") { inTextSegment = true; continue; }

        //remove labels
        if (cleaned.find(':') != string::npos)
         {
            cleaned = cleaned.substr(cleaned.find(':') + 1);
            cleaned = trim(cleaned);
        }
        if (cleaned.empty()) continue;

        if (inTextSegment)
         { //only process lines if we're in .text
            //seperate the instruction operation and operands
            vector<string> operands = parseOperands(cleaned);
            if (operands.empty()) continue;
            //get instruction name that would be first element of operands
            string instName = operands[0];


            if (instructionMap.count(instName))
             {
                //get machine code
                uint32_t machineCode = assemble(instructionMap[instName], operands, currentAddress, symbolTable);
                //get compressed assembly string
                string compressedAsm = getCompressedAssembly(operands);
                //get debug string
                string debugString = getDebugString(instructionMap[instName], operands, lastOffset);

                //write to output file
                outputFile << Hexa(currentAddress, 0) << " " << Hexa(machineCode, 8) << " , " << compressedAsm << " " << debugString << endl;
                
                //next instruction address
                currentAddress += 4;
            } 
            else 
            {
                cerr << "warning-skipping unknown instruction '" << instName << "'" << endl;
            }
        }
    }

    outputFile << Hexa(currentAddress, 0) << " 0xENDDC0DE" << " End of text segment" << endl;
//data segment
    inputFile_pass2.clear(); //clear the End-of-File flag
    inputFile_pass2.seekg(0); //rewind the file to the beginning
    
    //now we will work on the data segment

    dataAddress = 0x10000000; //reset data address
    inTextSegment = true;     //reset flag to find the .data directive
    bool wroteDataHeader = false;

    while (getline(inputFile_pass2, line)) 
    {
        string cleaned = cleanLine(line);

        if (cleaned == ".data") { inTextSegment = false; continue; }
        if (cleaned == ".text") { inTextSegment = true; continue; }
        //remove labels
        if (cleaned.find(':') != string::npos) 
        {
            cleaned = cleaned.substr(cleaned.find(':') + 1);
            cleaned = trim(cleaned);
        }
        if (cleaned.empty()) continue;

        if (!inTextSegment)
         { //process lines if we're in .data
            
            //add a line to separate text and data segment
            if (!wroteDataHeader) 
            {
                outputFile << endl; // Add a blank line for spacing
                wroteDataHeader = true;
            }

            //separate the directive and operands
            vector<string> operands = parseOperands(cleaned);

            if (operands.empty()) continue;
            string directive = operands[0];
            
            //print data based on directive
            if (directive == ".asciz") 
            {
                outputFile << Hexa(dataAddress, 0) << " ";
                size_t fq = line.find('\"'), lq = line.rfind('\"');
                if (fq != string::npos && lq != string::npos && fq != lq) 
                {
                    string strData = line.substr(fq + 1, lq - fq - 1);

                    //null char at the end of string
                    outputFile << "\"" << strData << "\\0\""; // Show string
                    dataAddress += strData.length() + 1;
                }
                outputFile << endl;
            } 
            else 
            { 
                string valueStr = operands[1];
                long value = stringToLong(valueStr);
                outputFile << Hexa(dataAddress, 0) << " ";

                if (directive == ".byte")
                 {
                    outputFile << Hexa(static_cast<uint32_t>(value & 0xFF), 2);
                    dataAddress += 1;
                } 
                else if (directive == ".half")
                 {
                    outputFile << Hexa(static_cast<uint32_t>(value & 0xFFFF), 4);
                    dataAddress += 2;
                } 
                else if (directive == ".word")
                 {
                    outputFile << Hexa(static_cast<uint32_t>(value & 0xFFFFFFFF), 8);
                    dataAddress += 4;
                } 
                else if (directive == ".dword")
             {
                    outputFile << Hexa(static_cast<uint64_t>(value), 16);
                    dataAddress += 8;
                }
                outputFile << endl;
            }
        }
    }

    inputFile_pass2.close();
    outputFile.close();

    cout << "Pass 2 complete. Output written to " << outputFilename << endl;
    return 0;
}