Name: uneven
Version: 1.0.0
Release: 1%{?dist}
Summary: Microphone volume unleveler
Group: Applications/Multimedia
License: GPLv3+
Url: https://github.com/ASzc/uneven
Source0: https://github.com/ASzc/uneven/archive/uneven-%{version}.tar.gz
BuildRequires: pulseaudio-libs-devel >= 3.0

%description
Uneven will efficiently enforce a constant microphone volume level, to counter
annoying volume "leveling" adjustments performed by e.g. BlueJeans.

%prep
%setup -q -n uneven-uneven-%{version}

%build
sh build.sh

%install
install -Dm755 uneven %{buildroot}%{_bindir}/uneven

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_bindir}/uneven
%doc README.md LICENSE

%changelog
* Mon Feb 29 2016 Alex Szczuczko <aszczucz@redhat.com> - 1.0.0-1
- Initial package
