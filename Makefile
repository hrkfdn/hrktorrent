include vars.mk

all: $(OUT)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OUT): $(OBJ)
	$(CXX) $(LDFLAGS) $(OBJ) $(LIBS) -o $(OUT)

clean:
	rm -rf $(OBJ) $(OUT)

install: all
	@mkdir -p ${PREFIX}/bin
	@cp -f hrktorrent ${PREFIX}/bin
	@chmod 755 ${PREFIX}/bin/hrktorrent
	@mkdir -p ${PREFIX}/share/examples/hrktorrent
	@cp -f hrktorrent.rc.example ${PREFIX}/share/examples/hrktorrent/hrktorrent.rc
	@chmod 644 ${PREFIX}/share/examples/hrktorrent
	@mkdir -p ${MANPREFIX}/man1
	@cp -f hrktorrent.1  ${MANPREFIX}/man1/hrktorrent.1
	@chmod 644 ${MANPREFIX}/man1/hrktorrent.1

uninstall:
	@rm -f ${PREFIX}/bin/hrktorrent
	@rm -f ${MANPREFIX}/man1/hrktorrent.1
	@rm -r ${PREFIX}/share/examples/hrktorrent/hrktorrent.rc
