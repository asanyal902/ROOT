##===- docs/Makefile ---------------------------------------*- Makefile -*-===##
# 
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
# 
##===----------------------------------------------------------------------===##

LEVEL      := ..
DIRS       :=

ifdef BUILD_FOR_WEBSITE
PROJ_OBJ_DIR = .
DOXYGEN = doxygen

$(PROJ_OBJ_DIR)/doxygen.cfg: doxygen.cfg.in
	cat $< | sed \
	  -e 's/@abs_top_srcdir@/../g' \
	  -e 's/@DOT@/dot/g' \
	  -e 's/@PACKAGE_VERSION@/mainline/' \
	  -e 's/@abs_top_builddir@/../g' > $@
endif

include $(LEVEL)/Makefile.common

HTML       := $(wildcard $(PROJ_SRC_DIR)/*.html) \
              $(wildcard $(PROJ_SRC_DIR)/*.css)
DOXYFILES  := doxygen.cfg.in doxygen.css doxygen.footer doxygen.header \
              doxygen.intro
EXTRA_DIST := $(HTML) $(DOXYFILES) llvm.css CommandGuide

.PHONY: install-html install-doxygen doxygen install-ocamldoc ocamldoc generated

install_targets := install-html
ifeq ($(ENABLE_DOXYGEN),1)
install_targets += install-doxygen
endif
ifdef OCAMLDOC
ifneq (,$(filter ocaml,$(BINDINGS_TO_BUILD)))
install_targets += install-ocamldoc
endif
endif
install-local:: $(install_targets)

generated_targets := doxygen
ifdef OCAMLDOC
generated_targets += ocamldoc
endif

# Live documentation is generated for the web site using this target:
# 'make generated BUILD_FOR_WEBSITE=1'
generated:: $(generated_targets)

install-html: $(PROJ_OBJ_DIR)/html.tar.gz
	$(Echo) Installing HTML documentation
	$(Verb) $(MKDIR) $(DESTDIR)$(PROJ_docsdir)/html
	$(Verb) $(DataInstall) $(HTML) $(DESTDIR)$(PROJ_docsdir)/html
	$(Verb) $(DataInstall) $(PROJ_OBJ_DIR)/html.tar.gz $(DESTDIR)$(PROJ_docsdir)

$(PROJ_OBJ_DIR)/html.tar.gz: $(HTML)
	$(Echo) Packaging HTML documentation
	$(Verb) $(RM) -rf $@ $(PROJ_OBJ_DIR)/html.tar
	$(Verb) cd $(PROJ_SRC_DIR) && \
	  $(TAR) cf $(PROJ_OBJ_DIR)/html.tar *.html
	$(Verb) $(GZIPBIN) $(PROJ_OBJ_DIR)/html.tar

install-doxygen: doxygen
	$(Echo) Installing doxygen documentation
	$(Verb) $(MKDIR) $(DESTDIR)$(PROJ_docsdir)/html/doxygen
	$(Verb) $(DataInstall) $(PROJ_OBJ_DIR)/doxygen.tar.gz $(DESTDIR)$(PROJ_docsdir)
	$(Verb) cd $(PROJ_OBJ_DIR)/doxygen && \
	  $(FIND) . -type f -exec \
	    $(DataInstall) {} $(DESTDIR)$(PROJ_docsdir)/html/doxygen \;

doxygen: regendoc $(PROJ_OBJ_DIR)/doxygen.tar.gz

regendoc:
	$(Echo) Building doxygen documentation
	$(Verb) if test -e $(PROJ_OBJ_DIR)/doxygen ; then \
	  $(RM) -rf $(PROJ_OBJ_DIR)/doxygen ; \
	fi
	$(Verb) $(DOXYGEN) $(PROJ_OBJ_DIR)/doxygen.cfg

$(PROJ_OBJ_DIR)/doxygen.tar.gz: $(DOXYFILES) $(PROJ_OBJ_DIR)/doxygen.cfg
	$(Echo) Packaging doxygen documentation
	$(Verb) $(RM) -rf $@ $(PROJ_OBJ_DIR)/doxygen.tar
	$(Verb) $(TAR) cf $(PROJ_OBJ_DIR)/doxygen.tar doxygen
	$(Verb) $(GZIPBIN) $(PROJ_OBJ_DIR)/doxygen.tar
	$(Verb) $(CP) $(PROJ_OBJ_DIR)/doxygen.tar.gz $(PROJ_OBJ_DIR)/doxygen/html/

userloc: $(LLVM_SRC_ROOT)/docs/userloc.html

$(LLVM_SRC_ROOT)/docs/userloc.html:
	$(Echo) Making User LOC Table
	$(Verb) cd $(LLVM_SRC_ROOT) ; ./utils/userloc.pl -details -recurse \
	  -html lib include tools runtime utils examples autoconf test > docs/userloc.html

install-ocamldoc: ocamldoc
	$(Echo) Installing ocamldoc documentation
	$(Verb) $(MKDIR) $(DESTDIR)$(PROJ_docsdir)/ocamldoc/html
	$(Verb) $(DataInstall) $(PROJ_OBJ_DIR)/ocamldoc.tar.gz $(DESTDIR)$(PROJ_docsdir)
	$(Verb) cd $(PROJ_OBJ_DIR)/ocamldoc && \
	  $(FIND) . -type f -exec \
	    $(DataInstall) {} $(DESTDIR)$(PROJ_docsdir)/ocamldoc/html \;

ocamldoc: regen-ocamldoc
	$(Echo) Packaging ocamldoc documentation
	$(Verb) $(RM) -rf $(PROJ_OBJ_DIR)/ocamldoc.tar*
	$(Verb) $(TAR) cf $(PROJ_OBJ_DIR)/ocamldoc.tar ocamldoc
	$(Verb) $(GZIPBIN) $(PROJ_OBJ_DIR)/ocamldoc.tar
	$(Verb) $(CP) $(PROJ_OBJ_DIR)/ocamldoc.tar.gz $(PROJ_OBJ_DIR)/ocamldoc/html/

regen-ocamldoc:
	$(Echo) Building ocamldoc documentation
	$(Verb) if test -e $(PROJ_OBJ_DIR)/ocamldoc ; then \
		$(RM) -rf $(PROJ_OBJ_DIR)/ocamldoc ; \
	fi
	$(Verb) $(MAKE) -C $(LEVEL)/bindings/ocaml ocamldoc
	$(Verb) $(MKDIR) $(PROJ_OBJ_DIR)/ocamldoc/html
	$(Verb) \
		$(OCAMLDOC) -d $(PROJ_OBJ_DIR)/ocamldoc/html -sort -colorize-code -html \
		`$(FIND) $(LEVEL)/bindings/ocaml -name "*.odoc" -exec echo -load '{}' ';'`

uninstall-local::
	$(Echo) Uninstalling Documentation
	$(Verb) $(RM) -rf $(DESTDIR)$(PROJ_docsdir)
