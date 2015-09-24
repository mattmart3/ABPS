SOURCE:=ted_current

all: ../${SOURCE}.pdf

.PHONY: all clean

# Execute pdflatex 3 times, workaround to get working \label and \ref tags.
../%.pdf: ${SOURCE}.tex
	pdflatex $^ && pdflatex $^ && pdflatex $^ && (rm *.log *.out || true) && mv ${SOURCE}.pdf $@

clean:
	rm *.aux *.log *.out ../${SOURCE}.pdf 2> /dev/null || true
