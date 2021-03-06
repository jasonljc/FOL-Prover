#include"Parser.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>

using namespace std;

// #define DEBUG_PARSE         // control log
/* The original clauses are in string format. This file is saving the clauses into a tree structure.
   The tree structure helps to convert the clauses to CNF and furthur save in a more compact way (structures defined in header file).
   */

FolParser::FolParser(vector<string> folkb, vector<string> folq){
    /* Parse, convert and archive queries and kb.
    */
    for(string s : folkb){
        s.erase(remove(s.begin(),s.end(),' '), s.end());
        #ifdef DEBUG_PARSE
        cout<<"--------------parsing "<<s<<endl;
        #endif
        TreeNode* head = new TreeNode();
        head->r = s.size()-1;
        parse(s, head); // Parse the string to a simple tree.
        convert(head); // Some steps to convert the tree into CNF
        #ifdef DEBUG_PARSE
        cout<<"--------------archiving"<<endl;
        #endif
        archiveKB(head, NULL); // Put the CNF into our knowledge base.
        delete head;
    }
    
    for(string s : folq){
        s.erase(remove(s.begin(),s.end(),' '), s.end());
        cout<<"--------------parsing "<<s<<endl;
        TreeNode* head = new TreeNode();
        head->r = s.size()-1;
        parse(s, head); 
        negate(head); // Negate the queries before archiving.
        convert(head);
        #ifdef DEBUG_PARSE
        cout<<"--------------archiving"<<endl;
        #endif
        archiveQ(head, NULL); // Put the CNF into our query base.
        delete head;
    }
    #ifdef DEBUG_PARSE
    cout<<"**********************"<<endl;
    cout<<"***Parsing complete***"<<endl;
    cout<<"**********************"<<endl;
    #endif
}

FolParser::~FolParser(){

}

KB FolParser::getKB(){
    return kb;
}

vector<Clause> FolParser::getQuery(){
    return queries;
}

// set a node first and use it as a container
// 
// tn ->  "string"    <- check if can be splitted 
//          /  \      <- create nodes for substring now
//     "sub1"  "sub2"
// 
bool FolParser::parse(string &s, TreeNode* tn){
    /* Convert a string to a tree. Recursively called.
    */
    int leftsemi = 0;
    /* five possible parsing:
        not
        implication
        or
        and
        default!
        ----------------------
        start with not (only need to look at the first char)
    */ 
    if(s.at(tn->l)=='~'){
        #ifdef DEBUG_PARSE
        cout<<"not "<<tn->l<<" "<<tn->r<<endl;
        #endif
        tn->op = OP_NOT;
        TreeNode* tn1 = new TreeNode();             //creating new node
        tn->left = tn1;
        tn->right = NULL;
        tn1->l = tn->l+1;
        tn1->r = tn->r;
        parse(s, tn1);
        return true;
    }
    if(s.at(tn->l+1)=='~'){
        #ifdef DEBUG_PARSE
        cout<<"not "<<tn->l<<" "<<tn->r<<endl;
        #endif
        tn->op = OP_NOT;
        TreeNode* tn1 = new TreeNode();             //creating new node
        tn->left = tn1;
        tn->right = NULL;
        tn1->l = tn->l+2;
        tn1->r = tn->r-1;
        parse(s, tn1);
        return true;
    }
    for(int i = tn->l+1; i< tn->r-1; ++i){
        if(s[i] == ' ') continue;
        if(leftsemi == 0){
            if(s[i] == '&'){
                #ifdef DEBUG_PARSE
                cout<<"and"<<endl;
                #endif
                tn->op = OP_AND;
                TreeNode *tn1 = new TreeNode(), *tn2 = new TreeNode();             //creating new node
                tn->left = tn1;
                tn->right = tn2;
                tn1->l = tn->l+1;
                tn1->r = i-1;
                tn2->l = i+1;
                tn2->r = tn->r-1;
                parse(s, tn1);
                parse(s, tn2);
                return true;
            }else if(s[i] == '|'){
                #ifdef DEBUG_PARSE
                cout<<"or"<<endl;
                #endif
                tn->op = OP_OR;
                TreeNode *tn1 = new TreeNode(), *tn2 = new TreeNode();             //creating new node
                tn->left = tn1;
                tn->right = tn2;
                tn1->l = tn->l+1;
                tn1->r = i-1;
                tn2->l = i+1;
                tn2->r = tn->r-1;
                parse(s, tn1);
                parse(s, tn2);
                return true;
            }else if(s[i] == '='){
                #ifdef DEBUG_PARSE
                cout<<"imply"<<endl;
                #endif
                tn->op = OP_IMPLY;
                TreeNode *tn1 = new TreeNode(), *tn2 = new TreeNode();
                tn->left = tn1;
                tn->right = tn2;
                tn1->l = tn->l+1;
                tn1->r = i-1;
                tn2->l = i+2;
                tn2->r = tn->r-1;
                parse(s, tn1);
                parse(s, tn2);
                return true;
            }
        }
        if(s[i] == '('){
            leftsemi++;
        }else if(s[i] == ')'){
            leftsemi--;
        }
    }
    tn->op = OP_DEF;
    tn->left = NULL;
    tn->right = NULL;
    if(s[tn->l] == '('){
        ++tn->l;
        --tn->r;
    }
    tn->context = s.substr(tn->l, tn->r - tn->l+1);
    #ifdef DEBUG_PARSE
    cout<<"def: "<<tn->context<<endl;
    #endif
    return true;
}

bool FolParser::convert(TreeNode*& tn){
    /* Some steps to convert to CNF
    */
    #ifdef DEBUG_PARSE
    cout<<"--------------converting"<<endl;
    #endif
    impElim(tn);
    notInwd(tn);
    andDstb(tn);
    return true;
}

void FolParser::negate(TreeNode*& tn){
    /* Negate queries. Used in proving by contradiction.
    */
    #ifdef DEBUG_PARSE
    cout<<"--------------negating"<<endl;
    #endif
    TreeNode* newHead = new TreeNode();
    newHead->op = OP_NOT;
    newHead->left = tn;
    tn = newHead;                    //hey! passing the reference of a pointer
}

void FolParser::impElim(TreeNode* tn){
    if(!tn) return;
    if(tn->op == OP_IMPLY){
        #ifdef DEBUG_PARSE
        cout<<"imply eliminated"<<endl;
        #endif
        TreeNode* tempNot = new TreeNode();
        tempNot->left = tn->left;
        tempNot->op = OP_NOT;
        tn->op = OP_OR;
        tn->left = tempNot;
    }
    impElim(tn->left);
    impElim(tn->right);
}

void FolParser::notInwd(TreeNode*& tn){
    if(!tn) return;
    if(tn->op == OP_NOT){
        if(tn->left->op == OP_NOT){
            #ifdef DEBUG_PARSE
            cout<<"double not eliminated"<<endl;
            #endif
            // TreeNode *tdel = tn;                  // Node Creating
            tn = tn->left->left;
            
            // delete tdel, tdel->left;   // Many nodes to be deleted!!
        }else if(tn->left->op == OP_AND){
            #ifdef DEBUG_PARSE
            cout<<"not into and"<<endl;
            #endif
            TreeNode *tempNotLeft = new TreeNode(), *tempNotRight = new TreeNode();       // Node Creating
            tempNotLeft->left = tn->left->left;
            tempNotRight->left = tn->left->right;
            tempNotLeft->op = OP_NOT;
            tempNotRight->op = OP_NOT;
            tn->op = OP_OR;
            tn->left = tempNotLeft;
            tn->right = tempNotRight;
        }else if(tn->left->op == OP_OR){
            #ifdef DEBUG_PARSE
            cout<<"not into or"<<endl;
            #endif
            TreeNode *tempNotLeft = new TreeNode(), *tempNotRight = new TreeNode();       // Node Creating
            tempNotLeft->left = tn->left->left;
            tempNotRight->left = tn->left->right;
            tempNotLeft->op = OP_NOT;
            tempNotRight->op = OP_NOT;
            tn->op = OP_AND;
            tn->left = tempNotLeft;
            tn->right = tempNotRight;            
        }else if(tn->left->op == OP_DEF){  // must be default
            #ifdef DEBUG_PARSE
            cout<<"not into def"<<endl;
            #endif
            tn->context = tn->left->context;
            delete tn->left;
            tn->left = NULL;
            return;
        }else{
            cout<<"not Inward parsing error!"<<endl;
            return;
        }
        notInwd(tn);
    }else{
        notInwd(tn->left);
        notInwd(tn->right);
    }
}

void FolParser::andDstb(TreeNode* tn){
    while(andDstbRecur(tn));
}

bool FolParser::andDstbRecur(TreeNode* tn){
    if(!tn) return false;
    bool ret = false;
    if(tn->op == OP_OR){
        if(tn->left->op == OP_AND){
            #ifdef DEBUG_PARSE
            cout<<"left and distributed"<<endl;
            #endif
            ret = true;
            tn->op = OP_AND;
            tn->left->op = OP_OR;
            TreeNode* tempRight = new TreeNode();                        // Node Creating
            tempRight->op = OP_OR;
            tempRight->left = tn->left->right;
            tempRight->right = tn->right;
            tn->left->right = tn->right;
            tn->right = tempRight;
        }else if(tn->right->op == OP_AND){
            #ifdef DEBUG_PARSE
            cout<<"left and distributed"<<endl;
            #endif
            ret = true;
            tn->op = OP_AND;
            tn->right->op = OP_OR;
            TreeNode* tempLeft = new TreeNode();                         // Node Creating
            tempLeft->op = OP_OR;
            tempLeft->left = tn->left;
            tempLeft->right = tn->right->left;
            tn->right->left = tn->left;
            tn->left = tempLeft;
        }
    }
    if(andDstbRecur(tn->left)) ret = true;                                  // Unsure about this logic
    if(andDstbRecur(tn->right)) ret = true;
    return ret;
}

void FolParser::archiveKB(TreeNode* tn, Clause* c){
    /* Save the tree as a Clause structure and put into KB.
    */
    if(!tn) return;
    if(tn->op == OP_AND){
        archiveKB(tn->left, c);
        archiveKB(tn->right, c);
        return;
    }
    if(!c){
        #ifdef DEBUG_PARSE
        cout<<"adding clause to KB"<<endl;
        #endif
        c = new Clause();
        kb.clauses.push_back(*c);
        c = &(kb.clauses.back());
    }
    if(tn->op == OP_OR){
        archiveKB(tn->left, c);
        archiveKB(tn->right, c);
        
        return;
    }
    if(tn->op == OP_NOT || tn->op == OP_DEF){
        Literal l = parseLiteral(tn);
        int lpos = c->literals.size();
        int lind = kb.clauses.size()-1;
        if(kb.index.find(l.predicate)!=kb.index.end()){
            kb.index[l.predicate].clausepos.push_back(lpos);
            kb.index[l.predicate].clauseind.push_back(lind);
            kb.index[l.predicate].size++;
        }else{
            Mapping m;
            m.size = 1;
            m.clausepos.push_back(lpos);
            m.clauseind.push_back(lind);
            kb.index.insert({l.predicate, m});
        }
        c->literals.push_back(l);        // update the kb index here.
    }
}

void FolParser::archiveQ(TreeNode* tn, Clause* c){
    if(!tn) return;
    if(tn->op == OP_AND){
        archiveQ(tn->left, c);
        archiveQ(tn->right, c);
        return;
    }
    if(!c){
        #ifdef DEBUG_PARSE
        cout<<"adding clause to Q"<<endl;
        #endif
        c = new Clause();
        queries.push_back(*c);
        c = &(queries.back());
    }
    if(tn->op == OP_OR){
        archiveQ(tn->left, c);
        archiveQ(tn->right, c);
        
        return;
    }
    if(tn->op == OP_NOT || tn->op == OP_DEF){
        // #ifdef DEBUG_PARSE
        // cout<<"******"<<tn->op<<endl;
        // #endif
        Literal l = parseLiteral(tn);
        c->literals.push_back(l); 
    }
}

Literal FolParser::parseLiteral(TreeNode* tn){
    Literal l;
    string s = tn->context;
    if(s.size() == 0) cout<<"Error! empty literal!!"<<endl;
    switch(tn->op){
        case OP_DEF:
            l.istrue = true;
            break;
        case OP_NOT:
            l.istrue = false;
            break;
        default:
            cout<<"Error! wrong literal!!"<<endl;
            break;
    }
    for(int i= 0; i< s.size(); ++i){
        if(s[i] == '('){
            l.predicate = s.substr(0,i);                //  the problem is: how to update kb index mapping??
            ++i;
            for(int j = i; j< s.size(); ++j){
                if(s[j] == ',' || s[j] == ')'){     //j == s.size()-1 won't work when there is a tab or something
                    Argument a;
                    string temps = s.substr(i, j-i);
                    i = j+1;
                    if(isupper(temps[0])) a.isvariable = false;
                    else a.isvariable = true;
                    a.id = temps;
                    l.arguments.push_back(a);
                    if(s[j] == ')') break;
                }
            }
            break;
        }
    }
    return l;
}


// For testing
// int main(){
//     /*
//         read file
//         */
//     ifstream t("./test.txt");
//     std::vector<string> folkb, folq;
    
//     /*
//         save the lines of kb and query
//         */
//     int lines_kb, lines_q;
//     string s_lines_kb, s_lines_q;
    
//     if(!getline(t, s_lines_q)) return 1;
//     lines_q =stoi(s_lines_q);
    
//     folq.reserve(lines_q);
//     for(int i = 0; i<lines_q; ++i){
//         string temp_s;
//         getline(t, temp_s);
//         folq.push_back(temp_s);
//     }
    
//     getline(t, s_lines_kb);
//     lines_kb =stoi(s_lines_kb);
    
//     folkb.reserve(lines_kb);
//     for(int i = 0; i<lines_kb; ++i){
//         string temp_s;
//         getline(t, temp_s);
//         folkb.push_back(temp_s);
//     }
    
//     /*
//         let it be parsed into defined cnf structs
//         */
//     FolParser *parser = new FolParser(folkb,folq);
//     KB kb = parser->getKB();
//     vector<Clause> queries = parser->getQuery();
    

//     return 0;
// }
