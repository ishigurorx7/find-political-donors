VPATH = src include
CPPFLAGS = -I include

find_political_donors: find_political_donors.o FEC_data_acc.o FEC_metadata.o FEC_list.o misc_util.o
find_political_donors.o: find_political_donors.c err_number.h  FEC_data_acc.h  FEC_list.h  FEC_metadata.h  misc_util.h
FEC_data_acc.o: FEC_data_acc.c err_number.h  FEC_data_acc.h  FEC_list.h  FEC_metadata.h  misc_util.h
FEC_metadata.o: FEC_metadata.c err_number.h  FEC_data_acc.h  FEC_list.h  FEC_metadata.h  misc_util.h
FEC_list.o: FEC_list.c err_number.h  FEC_list.h  FEC_metadata.h
misc_util.o: misc_util.c err_number.h

%.o: %.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<

.PHONY: clean
clean:
	rm -f *.o find_political_donors

