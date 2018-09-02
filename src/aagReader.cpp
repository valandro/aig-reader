#include "aagReader.h"
#include <string.h>
#include <algorithm>
#include <unistd.h>

AAGReader::AAGReader(string sourcePath)
{
    // Parsing the file name
    file_name = sourcePath.c_str();
    size_t dot = file_name.find(".aag");
    file_name = file_name.substr(6,dot-6);
    // Test if file exists then close it, to open again
    std::ifstream ifile(sourcePath.c_str());
    if((bool)ifile){
        source.open(sourcePath.c_str());
    } else {
        cout << "File does not exist!!" << endl << endl;
    }
}

Aig* AAGReader::readFile()
{
    Aig* aig = new Aig();
    aig->setName(file_name);

    //Treating header
    source.getline(buf, 250, '\n');
    string s=buf;
    istringstream line;
    line.str(s);
    line >> word;

    if(strcmp("aag",word.c_str())!=0)
    {
        debug << "the file is not an AAG file!";
        return NULL;
    }

    int nNodes, nInputs, nFFs, nOutputs, nAnds;
    line >> word;
    nNodes = atoi(word.c_str());
    line >> word;
    nInputs = atoi(word.c_str());
    line >> word;
    nFFs = atoi(word.c_str());
    line >> word;
    nOutputs = atoi(word.c_str());
    line >> word;
    nAnds = atoi(word.c_str());

    if (nNodes != nInputs + nFFs + nAnds) {
        debug << "Wrong file header";
        return NULL;
    }

    if (nFFs != 0) {
        debug << "FF not supported yet";
        return NULL;
    }

    debug << s << "\nThe file header is ok!\n\n";

    AigNode** nodes = new AigNode*[nNodes + 1];
    OutputNode* outs = new OutputNode[nOutputs];
    InputNode* ins = new InputNode[nInputs];

    //Treating inputs
    for (int i = 1; i <= nInputs; i++) {
        // Get input value
        source.getline(buf, sizeof(int), '\n');
        string s = buf;
        istringstream line;
        line.str(s);
        line >> word;
        // Set input label name
        string name = word;
        InputNode *input = new InputNode();
        input->setName(name);
        aig->insertInputNode(input);
        aig->insertNode(input);
        debug << word << "\n";
    }

    //Treating outputs
    debug << "\n";
    for (int i = 1; i <= nOutputs; i++) {        
        // Get ouput value
        source.getline(buf, sizeof(int), '\n');
        string s = buf;
        istringstream line;
        line.str(s);
        line >> word;
        // Set output label name
        string name = word;
        OutputNode *output = new OutputNode();
        output->setName(name);
        aig->insertOutputNode(output);
        aig->insertNode(output);
        debug << word << "\n";
    }

    //Connecting ands
    debug << "\n";
    for (int i = 1; i <= nAnds; i++) {
        // Get ouput value
        vector<string> strings;
        source.getline(buf, 3 * sizeof(int),'\n');
        string s = buf;
        string c;
        istringstream line(s);
        // Parsing the AND line > [and] [and] [and]
        while(getline(line,c,' ')){
            strings.push_back(c);
        }
        AndNode* node_and = new AndNode();
        node_and->setName(strings[0]);
        // Connect AND to output
        OutputNode* output = this->findOutputNode(aig->getOutputs(),node_and->getName());
        if(output != NULL){
            int isOutputInverted = this->isInverted(output->getName());
            node_and->connectTo(output,0,isOutputInverted);
        }
        //Get input 1 pointer node
        InputNode* input1 = this->findInputNode(aig->getInputs(),strings[1]);
        if(input1 != NULL){
            int isInput1Inverted = this->isInverted(input1->getName());
            input1->connectTo(node_and,0,isInput1Inverted);
        } else {
            // So, the input 1 is a AND node
            AndNode* and1 = this->findAndNode(aig->getAndNodes(),strings[1]);
            if(and1 != NULL){
                int isAnd1Inverted = this->isInverted(and1->getName());
                and1->connectTo(node_and,0,isAnd1Inverted);
            }
        }
        //Get input 2 pointer node
        InputNode* input2 = this->findInputNode(aig->getInputs(),strings[2]);
        if(input2 != NULL){
            int isInput2Inverted = this->isInverted(input2->getName());
            input2->connectTo(node_and,1,isInput2Inverted);
        } else {
             // So, the input 2 is a AND node
            AndNode* and2 = this->findAndNode(aig->getAndNodes(),strings[2]);
            if(and2 != NULL){
                int isAnd2Inverted = this->isInverted(and2->getName());
                and2->connectTo(node_and,1,isAnd2Inverted);
            }
        }
        //Connect AND node to AIG
        aig->insertAndNode(node_and);
        aig->insertNode(node_and);
    }

    debug << "\n";
    string aigName;
    while(source)
    {
        source.getline(buf, 250, '\n');
        string s=buf;
        istringstream line;
        line.seekg(0);line.str(s);
        line >> word;

        if(strcmp("c",word.substr(0).c_str())==0){
            debug << "the comments began. Ignore the file from here!\n";
            break;
        } else if(strcmp(word.substr(0).c_str(),"i")==0){

        } else if(strcmp(word.substr(0).c_str(),"o")==0){

        } else if(strcmp(word.substr(0).c_str(),"l")==0){

        }
    }

    debug << "\ncreate the AIG and add all nodes\n";
    debug << "return the AIG";
    

    return aig;
}

// Function to find Input node on aig inputs, inverted or not;
InputNode* AAGReader::findInputNode(list<InputNode*> inputs, string label) {
    InputNode *node = NULL;
    size_t input_size = inputs.size();
    int node_label_int = stoi(label);
    // If node is inverted, i should change the label;
    if(node_label_int % 2 != 0){
        node_label_int--;
        label = std::to_string(node_label_int);
    }
    std::for_each(inputs.begin(), std::next(inputs.begin(),input_size),[&](InputNode* el) {
        const char* el_name = el->getName().c_str();
        if(strcmp(el_name,label.c_str()) == 0){
            node = el;
        }
    });
    return node;
}

// Function to find Output node on aig outputs, inverted or not;
OutputNode* AAGReader::findOutputNode(list<OutputNode*> outputs, string label) {
    OutputNode* node = NULL;
    string positive_label;
    string negative_label;
    size_t output_size = outputs.size();
    int node_label_int = stoi(label);

    // If node is inverted, i should get the positive_label;
    if(node_label_int % 2 != 0){
        node_label_int--;
        positive_label = std::to_string(node_label_int);
    } else {
        // If node is positive, i should get the negative_label;
        node_label_int++;
        negative_label = std::to_string(node_label_int);
    }
    std::for_each(outputs.begin(), std::next(outputs.begin(),output_size),[&](OutputNode* el) {
        const char* el_name = el->getName().c_str();
        if(strcmp(el_name,label.c_str()) == 0 || 
           strcmp(el_name,positive_label.c_str()) == 0 ||
           strcmp(el_name,negative_label.c_str()) == 0){
            node = el;
        }
    });
    return node;
}

// Function to find And node on aig ands, inverted or not;
AndNode* AAGReader::findAndNode(list<AndNode*> ands, string label) {
    AndNode* node = NULL;
    size_t ands_size = ands.size();
    int node_label_int = stoi(label);
    // If node is inverted, i should change the label;
    if(node_label_int % 2 != 0){
        node_label_int--;
        label = std::to_string(node_label_int);
    }
    std::for_each(ands.begin(), std::next(ands.begin(),ands_size),[&](AndNode* el) {
        const char* el_name = el->getName().c_str();
        if(strcmp(el_name,label.c_str()) == 0){
            node = el;
        }
    });
    return node;
}

// Function to discover if the node is positive or negative;
int AAGReader::isInverted(string label){
    int logic = stoi(label);
    if(logic % 2 == 0){
        return 0;
    } else {
        return 1;
    }
}