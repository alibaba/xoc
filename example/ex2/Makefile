EX2_OBJS += ex2.o

CFLAGS = -DFOR_DEX -D_DEBUG_ -D_LINUX_ -Wno-write-strings -Wsign-promo \
        -Wsign-compare -Wpointer-arith -Wno-multichar -Winit-self \
        -Wstrict-aliasing=3 -finline-limit=10000000 -Wswitch #-Wall
        #-Werror=overloaded-virtual \

INC=-I ../../opt -I ../../com -I ../../dex -I .
%.o:%.cpp
	@echo "build $<"
	gcc $(CFLAGS) $(INC) -O2 -c -g2 $< -o $@

ex2_objs: $(EX2_OBJS)

ex2: ex2_objs 
	gcc $(EX2_OBJS) $(CFLAGS) -o ex2.exe -L../.. -lopt -lstdc++ -lm
	@echo "success!!"

clean:
	@find ../../../ -name "*.o" | xargs rm -f
	@find ../../../ -name "*.dot" | xargs rm -f
	@find ../../../ -name "*.exe" | xargs rm -f
	@find ../../../ -name "*.elf" | xargs rm -f
	@find ../../../ -name "*.out" | xargs rm -f
	@find ../../../ -name "*.tmp" | xargs rm -f
	@find ../../../ -name "*.vcg" | xargs rm -f
	@find ../../../ -name "*.cxx" | xargs rm -f
	@find ../../../ -name "*.asm" | xargs rm -f

