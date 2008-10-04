include vars.mk

all: $(OUT)

.cpp.o:
	@echo [CXX] $<
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT): $(OBJ)
	@echo [LD] $@
	@$(CXX) $(LDFLAGS) $(OBJ) $(LIBS) -o $(OUT)

clean:
	-rm -rf $(OBJ) $(OUT) *~

install: all
	@install hrktorrent ${PREFIX}/bin
	@mkdir -p ${PREFIX}/share/examples/hrktorrent
	@install -m 644 hrktorrent.rc.example ${PREFIX}/share/examples/hrktorrent/hrktorrent.rc
	@mkdir -p ${MANPREFIX}/man1
	@install -m 644 hrktorrent.1 ${MANPREFIX}/man1/hrktorrent.1

uninstall:
	-@rm -f ${PREFIX}/bin/hrktorrent
	-@rm -f ${MANPREFIX}/man1/hrktorrent.1
	-@rm -r ${PREFIX}/share/examples/hrktorrent/
