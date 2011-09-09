# Base name for main document
NAME=starspan

# Document:	
.PHONY: all pdf ps clean

$(NAME).pdf: $(NAME).tex $(NAME).bib
	pdflatex $<
	bibtex $(NAME)
	pdflatex $<
	rm -f *.log *.out *.aux *.dvi
	
clean:
	rm -f *.ps *.pdf

	
