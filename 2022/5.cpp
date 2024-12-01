// Compile-time implementation of part 2 without any functions, standard
// includes or constexpr. The output is given in the compiler error message.
// For instance:
//
// day5.cpp:213:60: error: aggregate ‘D<list<I<'T'>, I<'P'>, I<'W'>, I<'C'>, I<'G'>, I<'N'>, I<'C'>, I<'C'>, I<'G'> > > PART2_ANSWER’ has incomplete type and cannot be defined
//   213 | D<read_result<apply_moves<init_state, moves>::type>::type> PART2_ANSWER;
//       |                                                            ^~~~~~~~~~~~
//
// which means that the answer to the input hard-coded in day5-input.cpp is
// "TPWCGNCCG".
//
// Yes, I feel fine, why do you as-- typedef typename typedef typename typedef
// typename typedef typename typedef template typedef typename typedef template
// struct typename typedef typename typedef typename typedef typename typedef
// typename typedef typename struct typename

template <int N, int From, int To>
struct move {
	enum {
		n = N,
		from_index = From,
		to_index = To,
	};
};

template <char v>
struct I {
	enum {
		value = v,
	};
};

template <typename...>
struct list {};

template <int N, typename> struct list_index {};

template <typename Head, typename...Tail>
struct list_index<0, list<Head, Tail...>> {
	typedef Head type;
};

template <int N, typename Head, typename...Tail>
struct list_index<N, list<Head, Tail...>> {
	typedef typename list_index<N - 1, list<Tail...>>::type type;
};

template <typename, typename> struct concatenate;

template <typename... T>
struct concatenate<list<T...>, list<>> {
	typedef list<T...> type;
};

template <typename Head, typename... T, typename... U>
struct concatenate<list<T...>, list<Head, U...>> {
	typedef typename concatenate<list<T..., Head>, list<U...>>::type type;
};

template <int, typename>
struct drop;

template <>
struct drop<0, list<>> {
	typedef list<> type;
};

template <typename Head, typename... Tail>
struct drop<0, list<Head, Tail...>> {
	typedef list<Head, Tail...> type;
};

template <int N, typename Head, typename... Tail>
struct drop<N, list<Head, Tail...>> {
	typedef typename drop<N - 1, list<Tail...>>::type type;
};

template <int, typename>
struct take;

template <>
struct take<0, list<>> {
	typedef list<> type;
};

template <typename Head, typename ...Tail>
struct take<0, list<Head, Tail...>> {
	typedef list<> type;
};

template <int N, typename Head, typename ...Tail>
struct take<N, list<Head, Tail...>> {
	typedef typename concatenate<
		list<Head>,
		typename take<N - 1, list<Tail...>>::type
	>::type type;
};

template <int N, typename Replacement, typename List>
struct replace;

template <typename Replacement, typename Head, typename... Elems>
struct replace<0, Replacement, list<Head, Elems...>> {
	typedef typename concatenate<
		list<Replacement>,
		list<Elems...>
	>::type type;
};

template <int N, typename Replacement, typename Head, typename... Elems>
struct replace<N, Replacement, list<Head, Elems...>> {
	typedef typename concatenate<
		list<Head>,
		typename replace<N - 1, Replacement, list<Elems...>>::type
	>::type type;
};

// Move N elements from the start of Source to Dest.
template <int N, typename Source, typename Dest>
struct do_move {
	typedef typename drop<N, Source>::type source_result;

	typedef typename concatenate<
		typename take<N, Source>::type,
		Dest
	>::type dest_result;
};

template <typename Stacks, typename Move>
struct apply_move {
	// The -1's here are since our lists are zero-indexed, but the input
	// starts indexing at 1.

	typedef typename list_index<Move::from_index - 1, Stacks>::type source_stack;
	typedef typename list_index<Move::to_index - 1, Stacks>::type dest_stack;
	typedef do_move<Move::n, source_stack, dest_stack> mb;

	typedef typename replace<
		Move::from_index - 1,
		typename mb::source_result,
		typename replace<
			Move::to_index - 1,
			typename mb::dest_result,
			Stacks
		>::type
	>::type type;
};

template <typename Stacks, typename Moves>
struct apply_moves;

template <typename Stacks>
struct apply_moves<Stacks, list<>> {
	typedef Stacks type;
};

template <typename Stacks, typename Head, typename ...Tail>
struct apply_moves<Stacks, list<Head, Tail...>> {
	typedef typename apply_moves<
		typename apply_move<Stacks, Head>::type,
		list<Tail...>
	>::type type;
};

template <typename Stacks>
struct read_result;

template <>
struct read_result<list<>> {
	typedef list<> type;
};

template <typename Head, typename... Tail>
struct read_result<list<Head, Tail...>> {
	typedef typename concatenate<
		list<typename list_index<0, Head>::type>,
		typename read_result<list<Tail...>>::type
	>::type type;
};

template <typename... Tail>
struct read_result<list<list<>, Tail...>> {
	typedef typename read_result<list<Tail...>>::type type;
};

#include "5-input.cpp"

template <typename> struct D;
D<read_result<apply_moves<init_state, moves>::type>::type> PART2_ANSWER;
