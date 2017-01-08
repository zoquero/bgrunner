#
# Quick guide for building RPM on Ubuntu:
#
# $ apt-get install rpm
# $ mkdir -p ~/rpmbuild/SOURCES ~/rpmbuild/SPECS
# $ cp ....tar.gz ~/rpmbuild/SOURCES 
# $ rpmbuild -ba hellorpm.spec 
#

%define bgrunner_version 1.0

Summary:   Background jobs runner
Name:      bgrunner
Version:   %{bgrunner_version}
Release:   1%{?dist}
Source0:   %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}
# BuildRoot: %{_tmppath}/%{name}-%{version}-build
License:   GPLv3 (GNU General Public License v3)
Group:     Applications/System
URL:       https://github.com/zoquero/bgrunner/
Packager:  Angel Galindo Munoz <zoquero@gmail.com>

#########################################################################################
# Ubuntu
#########################################################################################
%if 0%{?ubuntu_version}

Description: Background jobs runner
More info can be found here: https://github.com/zoquero/bgrunner

%endif


#########################################################################################
# SuSE, openSUSE
#########################################################################################
%if 0%{?suse_version}

# Requires(post): info
# Requires(preun): info
%endif

%description 

Background jobs runner
More info can be found here: https://github.com/zoquero/bgrunner

%prep
%setup -q

%build
%configure
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/share/man/man1
gzip doc/bgrunner.1
make install DESTDIR=$RPM_BUILD_ROOT/usr/bin MANDIR=$RPM_BUILD_ROOT/usr/share/man/man1

%clean
rm -rf $RPM_BUILD_ROOT

%post
# /sbin/install-info %{_infodir}/%{name}.info %{_infodir}/dir || :

%preun
# if [ $1 = 0] ; then
# /sbin/install-info --delete %{_infodir}/%{name}.info %{_infodir}/dir || :
# fi

%files
%defattr(0644,root,root,0644)
%attr(0755, root, root) /usr/bin/%{name}
### Can't make it work on Ubuntu and SLES at the same time. It just works on SLES:
%attr(0644, root, root) /usr/share/man/man1/bgrunner.1.gz

# %files -f %{name}.lang
# %doc AUTHORS ChangeLog COPYING NEWS README THANKS TODO
# %{_mandir}/man1/hellorpm.1.gz
# %{_infodir}/%{name}.info.gz
# %{_bindir}/hellorpm

%changelog
* Sun Nov 13 2016 Angel <zoquero@gmail.com> 1.0
- Initial version of the package
# ORG-LIST-END-MARKER
