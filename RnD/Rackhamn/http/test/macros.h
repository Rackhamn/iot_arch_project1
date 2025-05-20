#ifndef C_MACROS_H
#define C_MACROS_H

#if 0
// 1. Macro to apply FUNC to each argument
#define APPLY(F, ...)  EXPAND(APPLY_N(F, __VA_ARGS__))

// 2. Expand macro arguments
#define EXPAND(x) x

// 3. Count number of arguments (up to 16)
#define COUNT_ARGS(...) \
	COUNT_ARGS_(__VA_ARGS__, 16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1)

#define COUNT_ARGS_(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...) N

// 4. Dispatch macro to correct APPLY_N
#define APPLY_N(F, ...) \
    EXPAND(APPLY_##COUNT_ARGS(__VA_ARGS__)(F, __VA_ARGS__)), 

// 5. Define APPLY_1 through APPLY_16
#define APPLY_1(F, A)               F(A)
#define APPLY_2(F, A, ...)          F(A), APPLY_1(F, __VA_ARGS__)
#define APPLY_3(F, A, ...)          F(A), APPLY_2(F, __VA_ARGS__)
#define APPLY_4(F, A, ...)          F(A), APPLY_3(F, __VA_ARGS__)
#define APPLY_5(F, A, ...)          F(A), APPLY_4(F, __VA_ARGS__)
#define APPLY_6(F, A, ...)          F(A), APPLY_5(F, __VA_ARGS__)
#define APPLY_7(F, A, ...)          F(A), APPLY_6(F, __VA_ARGS__)
#define APPLY_8(F, A, ...)          F(A), APPLY_7(F, __VA_ARGS__)
#define APPLY_9(F, A, ...)          F(A), APPLY_8(F, __VA_ARGS__)
#define APPLY_10(F, A, ...)         F(A), APPLY_9(F, __VA_ARGS__)
#define APPLY_11(F, A, ...)         F(A), APPLY_10(F, __VA_ARGS__)
#define APPLY_12(F, A, ...)         F(A), APPLY_11(F, __VA_ARGS__)
#define APPLY_13(F, A, ...)         F(A), APPLY_12(F, __VA_ARGS__)
#define APPLY_14(F, A, ...)         F(A), APPLY_13(F, __VA_ARGS__)
#define APPLY_15(F, A, ...)         F(A), APPLY_14(F, __VA_ARGS__)
#define APPLY_16(F, A, ...)         F(A), APPLY_15(F, __VA_ARGS__)
#endif



#if 0
// Expand macro args
#define EXPAND(x) x

// Count number of arguments (up to 9)
#define COUNT_ARGS(...) COUNT_ARGS_(__VA_ARGS__, 9,8,7,6,5,4,3,2,1)
#define COUNT_ARGS_(_1,_2,_3,_4,_5,_6,_7,_8,_9,N,...) N

// Dispatcher macro
#define FOREACH_COMMA(F, ...) EXPAND(FOREACH_COMMA_N(COUNT_ARGS(__VA_ARGS__), F, __VA_ARGS__))

// Call appropriate FOREACH_COMMA_N
#define FOREACH_COMMA_N(N, F, ...) EXPAND(FOREACH_COMMA_##N(F, __VA_ARGS__))

// Base case expansions
#define FOREACH_COMMA_1(F, A)             F(A)
#define FOREACH_COMMA_2(F, A, ...)        F(A), FOREACH_COMMA_1(F, __VA_ARGS__)
#define FOREACH_COMMA_3(F, A, ...)        F(A), FOREACH_COMMA_2(F, __VA_ARGS__)
#define FOREACH_COMMA_4(F, A, ...)        F(A), FOREACH_COMMA_3(F, __VA_ARGS__)
#define FOREACH_COMMA_5(F, A, ...)        F(A), FOREACH_COMMA_4(F, __VA_ARGS__)
#define FOREACH_COMMA_6(F, A, ...)        F(A), FOREACH_COMMA_5(F, __VA_ARGS__)
#define FOREACH_COMMA_7(F, A, ...)        F(A), FOREACH_COMMA_6(F, __VA_ARGS__)
#define FOREACH_COMMA_8(F, A, ...)        F(A), FOREACH_COMMA_7(F, __VA_ARGS__)
#define FOREACH_COMMA_9(F, A, ...)        F(A), FOREACH_COMMA_8(F, __VA_ARGS__)
#endif




#define FOREACH_1(F, x) F(x)

#define FOREACH_2(F, x, ...) F(x), FOREACH_1(F, __VA_ARGS__)
#define FOREACH_3(F, x, ...) F(x), FOREACH_2(F, __VA_ARGS__)
#define FOREACH_4(F, x, ...) F(x), FOREACH_3(F, __VA_ARGS__)
#define FOREACH_5(F, x, ...) F(x), FOREACH_4(F, __VA_ARGS__)
#define FOREACH_6(F, x, ...) F(x), FOREACH_5(F, __VA_ARGS__)
#define FOREACH_7(F, x, ...) F(x), FOREACH_6(F, __VA_ARGS__)
#define FOREACH_8(F, x, ...) F(x), FOREACH_7(F, __VA_ARGS__)
#define FOREACH_9(F, x, ...) F(x), FOREACH_8(F, __VA_ARGS__)
#define FOREACH_10(F, x, ...) F(x), FOREACH_9(F, __VA_ARGS__)
#define FOREACH_11(F, x, ...) F(x), FOREACH_10(F, __VA_ARGS__)
#define FOREACH_12(F, x, ...) F(x), FOREACH_11(F, __VA_ARGS__)
#define FOREACH_13(F, x, ...) F(x), FOREACH_12(F, __VA_ARGS__)
#define FOREACH_14(F, x, ...) F(x), FOREACH_13(F, __VA_ARGS__)
#define FOREACH_15(F, x, ...) F(x), FOREACH_14(F, __VA_ARGS__)
#define FOREACH_16(F, x, ...) F(x), FOREACH_15(F, __VA_ARGS__)

#define FOREACH_N(_16, _15, _14, _13, _12, _11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, N, ...) \
	FOREACH_##N

#define FOREACH(F, ...) \
	FOREACH_N(__VA_ARGS__, \
		16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 \
		)(F, __VA_ARGS__)


#endif /* C_MACROS_H */
