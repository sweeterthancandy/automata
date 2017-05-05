
#pragma warning(push, 4)
#pragma warning( disable : 4530 4127)

#include <map>
#include <set>
#include <iostream>
#include <algorithm>
#include <string>

struct node;

struct state{
        typedef std::set<node*>                  set_type;
        typedef std::set<node*>::iterator        iterator;
        typedef std::set<node*>::const_iterator  const_iterator;

private:
        typedef const_iterator                    CMI;
        typedef iterator                          MI;
public:

        iterator       begin()     { return mem_.begin(); }
        iterator       end  ()     { return mem_.  end(); }
        const_iterator begin()const{ return mem_.begin(); }
        const_iterator end  ()const{ return mem_.  end(); }
        size_t size()const{ return mem_.size(); }

        void insert(node* ptr){ mem_.insert(ptr); }

        state& operator+=(state const& that){
                for(CMI iter(that.mem_.begin()), end(that.mem_.end());iter!=end;++iter){
                        mem_.insert(*iter);
                }
                return *this;
        }
        state epsilon_closure()const;
private:
        std::set<node*> mem_;
};

struct node{

        explicit node(std::string const& tag):tag_(tag){}

        // decl transitions
        node& epsilon(node& that){
                return transition('\0', that);
        }
        node& transition(char c, node& that){
                edges[c].insert(&that);
                return *this;
        }

        state epsilon_closure(){
                state result;
                result.insert(this);
                result += edges['\0'].epsilon_closure();
                return result;
        }
        state const& closure(char c){
                return edges[c];
        }

        std::string const& tag()const{ return tag_; }

        //   0 are epsilon edges
        //   null nodes are ends
private:
        std::string tag_;
        std::map<char, state > edges;
};
        
inline
std::ostream& operator<<(std::ostream& ostr, state const& self){
        ostr << "{";
        for(state::const_iterator iter(self.begin()), end(self.end());iter!=end;++iter){
                ostr 
                        << ( iter == self.begin() ? "" : ", " )
                        << (*iter)->tag();
        }
        ostr << "}";
        return ostr;
}
state state::epsilon_closure()const{
        state result;
        for(CMI iter(mem_.begin()), end(mem_.end());iter!=end;++iter){
                result += (*iter)->epsilon_closure();
        }
        return result;
}


struct graph{
        graph(){
                #if 0
                for(char c=1;c!=0;++c){
                        (*this)["__end__"].transition(c, (*this)["__end__"]);
                }
                #endif
        }
        node& operator[](std::string const& tag){
                typedef std::map<std::string, node>::iterator MI;
                MI iter(nodes.find(tag));
                if( iter == nodes.end() )
                        iter = nodes.insert(std::make_pair(tag, node(tag))).first;
                return iter->second;
        }
        std::map<std::string, node> nodes;
};

bool match(graph& g, std::string const& seq){
        typedef state::iterator NI;

        state curr(g["__star__"].epsilon_closure());
        state next;

        std::cout << "BEGIN\n     curr = " << curr << "\n";
        for(size_t i=0;i!=seq.size();++i){
                char c(seq[i]);
                std::cout << "c=" << c << ", curr = " << curr << "\n";

                for(NI iter(curr.begin()),end(curr.end());iter!=end;++iter){
                        next += (*iter)->closure(c).epsilon_closure();
                }
                curr = next;
                next = state();
        }
        std::cout << "     curr = " << curr << "\nEND\n\n";

        for(NI iter(curr.begin()),end(curr.end());iter!=end;++iter){
                if( (*iter)->tag() == "__end__")
                        return true;
        }
        return false;
}

#if 0
struct factory{
        void register_(std::string const& expr, graph* g){
                m_[expr] = g;
        }
        graph* make(std::string const& expr){
                return m_[expr];
        }
        static factory& get(){
                factory* mem = 0;
                if( ! mem ){
                        mem = new factory;
                }
                return *mem;
        }
private:
        std::map<std::string, graph*> m_;
};
#endif





#define PRINT(EXPR) do{ std::cout << #EXPR << " => " << (EXPR) << "\n"; }while(0)

/*
        (ab|ba)
                 -epsilon---- 0 ---a--- 1 ----b-- 
        start  -/                                  \-- end
                \                                  /
                 -epsilon---- 2 ---b--- 4 ----a--
 */
void test0(){
        graph g;
        
        g["__star__"].epsilon( g["0"]);
        g["0"].transition('a', g["1"]);
        g["1"].transition('b', g["__end__"]);
        
        g["__star__"].epsilon( g["2"]);
        g["2"].transition('b', g["3"]);
        g["3"].transition('a', g["__end__"]);

        PRINT(match(g, ""));
        PRINT(match(g, "a "));
        PRINT(match(g, "ab"));
        PRINT(match(g, "ba"));
        PRINT(match(g, "abb"));
        PRINT(match(g, "bab"));
}
// a*(ab|ba)
void test1(){
        graph g;

        g["__star__"].transition('a', g["__star__"]);

        g["__star__"].epsilon( g["0"]);
        g["0"].transition('a', g["1"]);
        g["1"].transition('b', g["__end__"]);
        
        g["__star__"].epsilon( g["2"]);
        g["2"].transition('b', g["3"]);
        g["3"].transition('a', g["4"]);
        g["4"].epsilon(g["5"]);
        g["5"].epsilon(g["6"]);
        g["6"].epsilon(g["__end__"]);

        PRINT(match(g, ""));
        PRINT(match(g, "a "));
        PRINT(match(g, "ab"));
        PRINT(match(g, "ba"));
        PRINT(match(g, "aab"));
        PRINT(match(g, "aaba"));
        PRINT(match(g, "aabb"));
        PRINT(match(g, "abab"));
        PRINT(match(g, "baabb"));
        PRINT(match(g, "babab"));
}
//   a*b
//   (a|b)  

int main(){
        test0();
        std::cout << "-----\n";
        test1();
}



        
// vim:sw=8

