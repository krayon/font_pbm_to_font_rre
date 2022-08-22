all : font_pbm_to_font_rre

MAKE_TINY_x86_64:=-Os -s -flto -Wl,--relax -Wl,-Map=test.map -Wl,--gc-sections -ffunction-sections -fdata-sections -T./elf_x86_64_mod.x

font_pbm_to_font_rre : font_pbm_to_font_rre.o
	gcc $(MAKE_TINY_x86_64) -o $@ $^

vga8x12.font.h : font_pbm_to_font_rre vga8x12.pbm
	./font_pbm_to_font_rre vga8x12.pbm 8 16 > vga8x12.font.h

clean :
	rm -rf font_pbm_to_font_rre *.o *~

wipe : clean
	rm -rf test.map vga8x12.font.h
