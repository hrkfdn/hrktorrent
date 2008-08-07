PREFIX		= /usr/local
MANPREFIX	= ${PREFIX}/man
CXX?		= g++
OBJ		= main.o core.o settings.o utils.o ipfilter.o
OUT		= hrktorrent
CXXFLAGS	+= `pkg-config --cflags libtorrent`
LIBS		= `pkg-config --libs libtorrent` -lpthread

