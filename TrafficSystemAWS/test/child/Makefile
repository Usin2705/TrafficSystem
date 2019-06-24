#Macro definition
CC	= gcc 	#macro for selecting c complier
CFLAGS = -Wall #macro for complier flag
LDFLAG = -lpthread		#unused macro for linker flags
SOURCE = child.c #macro for defining source files
OBJFILES = $(SOURCE: .c=.o) #macro for defining object files
TARGET = child.exe		#macro for defining the target file files
CLEAR = &&clear

#Rules
$(TARGET): $(OBJFILES)		#Rule using macros to define dependencies
	$(CC) $(CFGLAG) -o $(TARGET) $(OBJFILES) $(LDFLAG) #macro for define C compilation command
clean:
	rm -f $(CLEAR)