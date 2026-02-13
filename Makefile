all:
	clang++ -o human_chess_engine_010_64_ja ./src/*.cpp -Isrc ./src/evaluation/*.cpp -Isrc/evaluation \
	./src/search/*.cpp -Isrc/search ./src/uci/*.cpp -Isrc/uci ./src/utils/*.cpp -Isrc/utils  -Ofast -w \
	-flto=auto -ftree-vectorize -funroll-loops -ffast-math -static -static-libgcc -static-libstdc++ -DNDEBUG \
	-finline-functions -pipe -std=c++17 -fno-rtti -fstrict-aliasing -fomit-frame-pointer -lm -fuse-ld=lld -MMD -MP -s \
	-funsafe-math-optimizations -fsee -pthread \
	-march=k8 -mtune=core2


   
	#       -march=x86-64-v3 -mtune=silvermont                                                                      <    avx/bmi2 enable 	
	
	
	#       -fprofile-instr-generate -fcoverage-mapping                                                             <   before -o
	
	#        llvm-profdata merge -output=default.profdata *.profraw                                                 <  enter on command line
 
    #       -fprofile-use=default.profdata                                                                          <   before -o
	
	
	
	#       -march=x86-64-v4 -mtune=silvermont                                                                      <   avx512 enable (slower than bmi2)
	                                     

    #       -march=znver2 -mtune=znver2                                                                             <  for Zen 2 processors (no bmi2)
    
    
    
    #       -march=sandybridge -mtune=silvermont                                                                    <  for avx1 processors (no avx2/bmi2)
    
    
    
    #       -march=silvermont -mtune=k8                                                                             <  for popcount builds (with sse4.1/4.2)    


    #       -march=k8 -mtune=core2                                                                                  <  sse3 build	
                  


                                                                    
