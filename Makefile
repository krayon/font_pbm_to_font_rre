all : fonter

MAKE_TINY_x86_64:=-Os -s -flto -Wl,--relax -Wl,-Map=test.map -Wl,--gc-sections -ffunction-sections -fdata-sections -T./elf_x86_64_mod.x

fonter : fonter.o
	gcc $(MAKE_TINY_x86_64) -o $@ $^

vga8x12.font.h : fonter vga8x12.pbm
	./fonter vga8x12.pbm 8 16 > vga8x12.font.h

clean :
	rm -rf fonter *.o *~

wipe : clean
	rm -rf vga8x12.font.h
