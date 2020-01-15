Name:       dali-layers
Summary:    DALi 3D Engine Validation Layers
Version:    1.4.47
Release:    1
Group:      System/Libraries
License:    Apache-2.0 and BSD-3-Clause and MIT
#URL:        https://review.tizen.org/git/?p=platform/core/uifw/dali-core.git;a=summary
#Source0:    %{name}-%{version}.tar.gz

Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  cmake
BuildRequires:  pkgconfig
BuildRequires:  gawk
Provides: libdali-layers.so

%if 0%{?tizen_version_major} >= 3
BuildRequires:  pkgconfig(libtzplatform-config)
%endif

%description
DALi 3D Engine Validation Layers

##############################
# Preparation
##############################
%prep
%setup -q

%define dev_include_path %{_includedir}

##############################
# Build
##############################
%build
PREFIX="/usr"
CXXFLAGS+=" -Wall -g -Os -DNDEBUG -fPIC -fvisibility-inlines-hidden -fdata-sections -ffunction-sections "
LDFLAGS+=" -Wl,--rpath=$PREFIX/lib -Wl,--as-needed -Wl,--gc-sections -lgcc_s -lgcc -Wl,-Bsymbolic-functions "

%ifarch %{arm}
CXXFLAGS+=" -D_ARCH_ARM_ -mfpu=neon"
%endif

libtoolize --force
cd %{_builddir}/%{name}-%{version}/build

CFLAGS="${CFLAGS:-%optflags}" ; export CFLAGS;
CXXFLAGS="${CXXFLAGS:-%optflags}" ; export CXXFLAGS;
LDFLAGS="${LDFLAGS:-%optflags}" ; export LDFLAGS;

cmake \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_INSTALL_PREFIX=%{_prefix} \
      -DCMAKE_INSTALL_LIBDIR=%{_libdir} \
      -DCMAKE_INSTALL_INCLUDEDIR=%{_includedir}

make %{?jobs:-j%jobs}

##############################
# Installation
##############################
%install
rm -rf %{buildroot}
cd build/tizen

pushd %{_builddir}/%{name}-%{version}/build/tizen
%make_install

# Create links to ensure linking with cxx11 library is preserved
pushd  %{buildroot}%{_libdir}
#ln -sf libdali-core.so libdali-layers-cxx11.so
#ln -sf libdali-core.so libdali-layers-cxx11.so.0
#ln -sf libdali-core.so libdali-layers-cxx11.so.0.0.0
popd

##############################
# Post Install
##############################
%post
/sbin/ldconfig
exit 0

##############################
# Post Uninstall
##############################
%postun
/sbin/ldconfig
exit 0

##############################
# Files in Binary Packages
##############################

%files
%if 0%{?enable_dali_smack_rules}
%manifest dali.manifest-smack
%else
%manifest dali.manifest
%endif
%defattr(-,root,root,-)
%{_libdir}/libdali-core-cxx11.so*
%{_libdir}/libdali-core.so*
%license LICENSE

%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/dali-core.pc
%{dev_include_path}/dali/public-api/*
%{dev_include_path}/dali/devel-api/*
%{dev_include_path}/dali/doc/*

%files integration-devel
%defattr(-,root,root,-)
%{_includedir}/dali/integration-api/*
