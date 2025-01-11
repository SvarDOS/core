$(WMINCRT): $(OBJS)
	*wlib -q -n -b $(WMINCRT) $(WLIB_OBJS)

.asm.obj:
	wasm -q  $(ASM_FLAGS) $<

clean: .symbolic
	rm -f *.obj
	rm -f *.lib
