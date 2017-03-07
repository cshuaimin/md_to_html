all: md_to_html

md_to_html: fake_dict.o md_to_html.o
	gcc -Wall fake_dict.o md_to_html.o -o md_to_html

fake_dict.o: fake_dict.c fake_dict.h
	gcc fake_dict.c -Wall -c -o fake_dict.o

md_to_html.o: md_to_html.c
	gcc md_to_html.c -Wall -c -o md_to_html.o

