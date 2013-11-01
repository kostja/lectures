
all:
	xsltproc --nonet --stringparam collect.xref.targets "all" \
	--xinclude -o schedule.html html.xsl schedule.xml

pdf:
	xsltproc --nonet --stringparam collect.xref.targets "all" \
	--xinclude -o schedule.fo fo.xsl schedule.xml 
	fop schedule.fo schedule.pdf
