sysconf_DATA = qadsl.conf

install-data-hook:									      \
	@list='$(sysconf_DATA)';							      \
	for p in $$list; do								      \
		dest=`echo $$p | sed -e s/.dist//`;					      \
		if test -f $(DESTDIR)$(sysconfdir)/$$dest; then				      \
			echo "$@ will not overwrite existing $(DESTDIR)$(sysconfdir)/$$dest"; \
		else									      \
			echo " $(INSTALL_DATA) $$p $(DESTDIR)$(sysconfdir)/$$dest";	      \
			$(INSTALL_DATA) $$p $(DESTDIR)$(sysconfdir)/$$dest;		      \
		fi;									      \
	done
