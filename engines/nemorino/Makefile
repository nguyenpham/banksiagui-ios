ifeq ($(OS),Windows_NT)
	EXE := _gcc.exe
else
	EXE := 
endif

ifeq ($(COMP),mingw)
	STATIC := -static
else
	STATIC :=
endif

FILES = board.cpp book.cpp evaluation.cpp hashtables.cpp Material.cpp \
    Nemorino.cpp position.cpp search.cpp settings.cpp test.cpp timemanager.cpp \
    uci.cpp utils.cpp tbprobe.cpp network.cpp

FLAGS = -O3 -Wmain -std=c++17 -flto -pthread -mavx2 -DNDEBUG -DUSE_AVX2 $(STATIC)

FLAGS_DEBUG = -Og -Wmain -std=c++17 -flto -pthread -mavx2 -DUSE_AVX2 -g $(STATIC)

FLAGS_BMI2 = -O3 -Wmain -std=c++17 -flto -pthread -DNDEBUG -DUSE_AVX2 -DPEXT -mavx2 -mbmi2 $(STATIC)

FLAGS_SSE41 = -O3 -Wmain -std=c++17 -flto -pthread -DNDEBUG -DUSE_SSE2 -DUSE_SSE41 -msse4.1 -msse2 $(STATIC)

FLAGS_SSE2 = -O3 -Wmain -std=c++17 -flto -pthread -DNDEBUG -DUSE_SSE2 -DNO_POPCOUNT -msse2 $(STATIC)

FLAGS_OLD_CPU = -O3 -Wmain -std=c++17 -flto -pthread -DNDEBUG -DNO_POPCOUNT -march=x86-64 $(STATIC)

make: $(FILES)
	g++ $(FLAGS) $(FILES) -o nemorino$(EXE)

bmi2: $(FILES)
	g++ $(FLAGS_BMI2) $(FILES) -o nemorino_bmi2$(EXE)

sse41: $(FILES)
	g++ $(FLAGS_SSE41) $(FILES) -o nemorino_sse41$(EXE)

sse2: $(FILES)
	g++ $(FLAGS_SSE2) $(FILES) -o nemorino_sse2$(EXE)

old_cpu: $(FILES)
	g++ $(FLAGS_OLD_CPU) $(FILES) -o nemorino_old_cpu$(EXE)

profile: $(FILES)
	g++ $(FLAGS_BMI2) -fprofile-generate $(FILES) -o nemorino_profile
	
bench: profile
	./nemorino_profile bench
	
pgo: bench
	g++ $(FLAGS_BMI2) -fprofile-use $(FILES) -o nemorino
	rm *.gcda
	rm nemorino_profile
	
debug: $(FILES)
	g++ $(FLAGS_DEBUG) $(FILES) -o $(EXE)