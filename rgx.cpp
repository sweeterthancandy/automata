


#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include <map>
#include <vector>
#include <set>
#include <iostream>
#include <algorithm>
#include <string>

#define PRINT(EXPR) do{ std::cout << #EXPR << " => " << (EXPR) << "\n"; }while(0)
#define EXPECT_BOOL(EXPR, VALUE)                                   \
do{                                                                \
	bool ret{ EXPR };				           \
	std::cout << #EXPR << " => " << ret << " "                 \
		<< ( ret == VALUE ? "Success" : "Failed" )         \
	<< "\n";						   \
}while(0)
#define EXPECT_TRUE(EXPR) EXPECT_BOOL(EXPR,true)
#define EXPECT_FALSE(EXPR) EXPECT_BOOL(EXPR,false)

struct nfa_node;

struct state{
        typedef std::set<const nfa_node*>                  set_type;
        typedef std::set<const nfa_node*>::iterator        iterator;
        typedef std::set<const nfa_node*>::const_iterator  const_iterator;

private:
        typedef const_iterator                    CMI;
        typedef iterator                          MI;
public:

        iterator       begin()     { return mem_.begin(); }
        iterator       end  ()     { return mem_.  end(); }
        const_iterator begin()const{ return mem_.begin(); }
        const_iterator end  ()const{ return mem_.  end(); }
        size_t size()const{ return mem_.size(); }
	auto empty()const{ return mem_.empty(); }

        void insert(nfa_node const* ptr){ mem_.insert(ptr); }

        state& operator+=(state const& that){
                for(CMI iter(that.mem_.begin()), end(that.mem_.end());iter!=end;++iter){
                        mem_.insert(*iter);
                }
                return *this;
        }
        bool operator==(state const& that)const{
                return mem_.size() == that.mem_.size() ||
                        std::equal(
                                begin(), end(),
                                that.begin(), that.end());
        }
        state epsilon_closure()const;
	state move(char c)const;

	friend state union_(state const& left, state const& right){
		std::vector<nfa_node const*> aux;
		std::set_intersection(
			left.begin(), left.end(),
			right.begin(), right.end(),
			std::back_inserter(aux));
		state result;
		for( auto _ : aux)
			result.insert(_);
		return std::move(result);
	}
	friend bool operator<(state const& left, state const& right){
		return std::lexicographical_compare(
			left.begin(), left.end(),
			right.begin(), right.end());
	}


private:
        std::set<const nfa_node*> mem_;
};

struct nfa_node{

        explicit nfa_node(std::string const& tag):tag_(tag){}

        // decl transitions
        nfa_node& epsilon(nfa_node* that){
                return transition('\0', that);
        }
        nfa_node& epsilon(nfa_node& that){
                return transition('\0', that);
        }
        nfa_node& transition(char c, nfa_node& that){
		return transition(c, &that);
        }
        nfa_node& transition(char c, nfa_node* that){
                edges[c].insert(that);
                return *this;
        }

        state epsilon_closure()const{
                state result;
                result.insert(this);
		auto iter = edges.find('\0');
		if( iter != edges.end() )
			result += iter->second.epsilon_closure();
                return result;
        }
        state closure(char c)const{
		state s;
		auto iter = edges.find(c);
		if( iter != edges.end())
			s = iter->second;
                return std::move(s);
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
state state::move(char c)const{
	state result;
	for( auto iter : mem_ ){
		result += iter->closure(c);
	}
	return std::move(result);
}


struct nfa_graph{

        nfa_graph(){
        }
	nfa_node* make(){
		// XXX unique not checked
		return get(boost::lexical_cast<std::string>(id_));
	}
	nfa_node* start(){ return get("__start__"); }
	nfa_node* end(){ return get("__end__"); }
	nfa_node const* start()const{ return get("__start__"); }
	nfa_node const* end()const{ return get("__end__"); }
        nfa_node* get(std::string const& tag){
                auto iter(nodes.find(tag));
                if( iter == nodes.end() )
                        iter = nodes.insert(std::make_pair(tag, nfa_node(tag))).first;
                return &iter->second;
        }
        nfa_node const* get(std::string const& tag)const{
                auto iter(nodes.find(tag));
                if( iter == nodes.end() )
			return nullptr;
                return &iter->second;
        }
private:
	unsigned id_ = 0;
        std::map<std::string, nfa_node> nodes;
};



bool match(nfa_graph const& g, std::string const& seq){
        typedef state::iterator NI;

	enum{
		Debug = 0
	};

        state curr(g.start()->epsilon_closure());

	state end;
	end.insert( g.end() );

	if(Debug)std::cout << "BEGIN\n     curr = " << curr << "\n";
        for(size_t i=0;i!=seq.size();++i){
                char c(seq[i]);

		curr = curr.move(c).epsilon_closure();
		
		if(Debug)std::cout << "c=" << c << ", curr = " << curr << "\n";
        }
	if(Debug)std::cout << "     curr = " << curr << "\nEND\n\n";

	return ! union_( curr, end).empty();

}



struct dfa_node{
	explicit dfa_node(std::string const& tag):tag_{tag}{}
	dfa_node* move(char c)const{
		auto iter = map_.find(c);
		return ( iter == map_.end() ? nullptr : iter->second );
	}
	dfa_node& transition(char c, dfa_node* ptr){
		map_.insert(std::make_pair(c,ptr));
		return *this;
	}
        void decl_accepting(){ accepting_ = true; }
        bool is_accepting()const{ return accepting_; }

        auto tag()const{ return tag_; }
private:
	std::string tag_;
	std::map<char, dfa_node*> map_;
        bool accepting_ = false;
};
struct dfa_graph{

        dfa_graph(){
        }
        void decl_start(dfa_node* ptr){ start_ = ptr; }
	dfa_node* start(){ return start_; }
	dfa_node const* start()const{ return start_; }
        dfa_node* get(std::string const& tag){
                auto iter(nodes.find(tag));
                if( iter == nodes.end() )
                        iter = nodes.insert(std::make_pair(tag, dfa_node(tag))).first;
                return &iter->second;
        }
        dfa_node const* get(std::string const& tag)const{
                auto iter(nodes.find(tag));
                if( iter == nodes.end() )
			return nullptr;
                return &iter->second;
        }
        void display()const{
                std::cout << boost::format("%5s %5s %5s %s\n") 
                        % "id" % "a" % "b" % " ";
                for( auto const& p : nodes){

                        auto ma = p.second.move('a');
                        auto mb = p.second.move('b');
                        std::cout << boost::format("%5s %5s %5s %s\n") 
                                % p.first
                                % ( ma ? ma->tag() : std::string{})
                                % ( mb ? mb->tag() : std::string{})
                                % ( p.second.is_accepting() ? "*" : "");
                }
        }
private:
        std::map<std::string, dfa_node> nodes;
        dfa_node* start_ = nullptr;
};

bool match(dfa_graph const& g, std::string const& seq){
        typedef state::iterator NI;

	enum{
		Debug = 0
	};

        dfa_node const* iter { g.start() };


	if(Debug)std::cout << "BEGIN\n     curr = " << iter->tag() << "\n";
        for(size_t i=0;i!=seq.size();++i){
                char c(seq[i]);

                iter = iter->move(c);
		
		if(Debug)std::cout << "c=" << c << ", curr = " << iter->tag() << "\n";
        }
	if(Debug)std::cout << "     curr = " << iter->tag() << "\nEND\n\n";

	return iter->is_accepting();
}


auto compile(nfa_graph const& nfa){
        dfa_graph dfa;

        state end;
        end.insert( nfa.end() );

	std::map< state, std::map<char, state > > m;
        std::map< state, std::string> id;

	std::vector<state> stack;
	//stack.emplace_back( nfa.start()->epsilon_closure() );

        auto start_ec{  nfa.start()->epsilon_closure() };      
        stack.emplace_back( start_ec );

	for(;stack.size();){
		auto head{ stack.back() };
		stack.pop_back();

                if( head.size() )
		for( char c : {'a', 'b' } ){
			if( m[head].count( c) == 1 )
				continue;
			state s{ head.move(c).epsilon_closure() };
                        m[head][c] = s;
                        //if( s.size() )
                        {
                                stack.push_back(s);
                        }
		}
	}
	PRINT(m.size());

        unsigned counter{0};

	for( auto const& k : m ){
                id[k.first] = boost::lexical_cast<std::string>(++counter);
                for( auto const& l : k.second){
                        std::cout << boost::format("%30s x %c -> %s\n")
                                % k.first
                                % l.first
                                % l.second;
                }
	}

        id[start_ec] = "__start__";
	
        for( auto const& k : m ) {

                dfa_node* node = dfa.get( id[k.first] );

                if( k.first == start_ec )
                        dfa.decl_start( node );
                
                if( union_( k.first, end ).size() )
                        node->decl_accepting();

                for( auto const& l : k.second){
                        node->transition(l.first, dfa.get( id[l.second] ) );

                }
        }

        return dfa;
}





/*
        a*(ab|ba)

        /-a-\
        |   |    -epsilon---- 0 ---a--- 1 ----b-- 
        start  -/                                  \-- end
                \                                  /
                 -epsilon---- 2 ---b--- 4 ----a--
 */
void test0(){
        nfa_graph g;

	auto start  = g.start();
	auto end = g.end();

	auto _0 = g.get("0");
	auto _1 = g.get("1");
	auto _2 = g.get("2");
	auto _3 = g.get("3");

	start->transition('a', start);

	start->epsilon(_0);
	_0->transition('a', _1);
	_1->transition('b', end);
	
	start->epsilon(_2);
	_2->transition('b', _3);
	_3->transition('a', end);
	
        auto dfa = compile(g);
	#if 0
	EXPECT_FALSE(match(g, ""));
        EXPECT_FALSE(match(g, "a"));
	EXPECT_TRUE(match(g, "ab"));
	EXPECT_TRUE(match(g, "ba"));
        EXPECT_FALSE(match(g, "abb"));
        EXPECT_FALSE(match(g, "bab"));
	EXPECT_TRUE(match(g, "aaab"));
	EXPECT_TRUE(match(g, "aba"));
	#endif
        dfa.display();
	
        EXPECT_FALSE(match(dfa, ""));
        EXPECT_FALSE(match(dfa, "a"));
	EXPECT_TRUE(match(dfa, "ab"));
	EXPECT_TRUE(match(dfa, "ba"));
        EXPECT_FALSE(match(dfa, "abb"));
        EXPECT_FALSE(match(dfa, "bab"));
	EXPECT_TRUE(match(dfa, "aaab"));
	EXPECT_TRUE(match(dfa, "aba"));

}

/*

        (a|b)*abb         ----<--- e -----------
                         /                      \
                        /   > e --<2>-- a --<3>  \
                       /   /                   \  \
        <start>-- e --<1>--                      -<6>-- e --<7>-- a --<8>-- b --<9> -- b <end>
             \             \                   /            /
              \             - e --<4>-- b --<5>            /
               \                                          /
                ------------->---- e --------------------
 */
void test1(){
        nfa_graph g;

	auto start  = g.start();
	auto end = g.end();

	auto _1 = g.get("1");
	auto _2 = g.get("2");
	auto _3 = g.get("3");
	auto _4 = g.get("4");
	auto _5 = g.get("5");
	auto _6 = g.get("6");
	auto _7 = g.get("7");
	auto _8 = g.get("8");
	auto _9 = g.get("9");

	start->epsilon(_1);
	start->epsilon(_7);

	_1->epsilon(_2);
	_1->epsilon(_4);

	_2->transition('a', _3);

	_3->epsilon(_6);

	_4->transition('b', _5);

	_5->epsilon(_6);

	_6->epsilon(_7);
	_6->epsilon(_1);

	_7->transition('a', _8);

	_8->transition('b', _9);

	_9->transition('b', end);

        auto dfa = compile(g);


        EXPECT_FALSE(match(g, ""));
        EXPECT_FALSE(match(g, "a"));
        EXPECT_FALSE(match(g, "ab"));
        EXPECT_FALSE(match(g, "ba"));
        EXPECT_TRUE(match(g, "abb"));
        EXPECT_FALSE(match(g, "bab"));
        EXPECT_FALSE(match(g, "aaab"));
        EXPECT_FALSE(match(g, "aba"));
        EXPECT_TRUE(match(g, "babb"));
        EXPECT_TRUE(match(g, "aaabb"));
        
        EXPECT_FALSE(match(dfa, ""));
        EXPECT_FALSE(match(dfa, "a"));
        EXPECT_FALSE(match(dfa, "ab"));
        EXPECT_FALSE(match(dfa, "ba"));
        EXPECT_TRUE(match(dfa, "abb"));
        EXPECT_FALSE(match(dfa, "bab"));
        EXPECT_FALSE(match(dfa, "aaab"));
        EXPECT_FALSE(match(dfa, "aba"));
        EXPECT_TRUE(match(dfa, "babb"));
        EXPECT_TRUE(match(dfa, "aaabb"));



}



int main(){
        test0();
	test1();
}



        
// vim:sw=8

