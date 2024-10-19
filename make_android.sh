#!/bin/bash

set -ex

# QT_ROOT
# SDK_ROOT
# NDK_ROOT
# QT_ANDROID_KEYSTORE_PATH
# QT_ANDROID_KEYSTORE_ALIAS
# QT_ANDROID_KEYSTORE_STORE_PASS
# LLVM_STRIP

export QT_HOST_PATH=$QT_ROOT/gcc_64
build_dir=build/android-release

if [ ! -e $build_dir/openssl ];then
mkdir -p $build_dir/openssl
pushd $build_dir/openssl

OPENSSL_VERSION=3.3.2
OPENSSL_SHA=2e8a40b01979afe8be0bbfb3de5dc1c6709fedb46d6c89c10da114ab5fc3d281

wget -Oconanfile.py https://github.com/conan-io/conan-center-index/raw/refs/heads/master/recipes/openssl/3.x.x/conanfile.py
replaced="        replace_in_file(self, os.path.join(self.source_folder, 'crypto/x509/by_dir.c'), 'X509_NAME_hash_ex(name, libctx, propq, \\&i);\\\\n    if (i == 0)', 'X509_NAME_hash_old(name);if(0)')"
sed -ie "s|strip_root=True)|strip_root=True)\n$replaced\n|" conanfile.py
sed -ie "s|shared_extension = \"\"|shared_extension = r'shared_extension => \".so\",'|" conanfile.py
cat <<EOF > conandata.yml
sources:
  ${OPENSSL_VERSION}:
    url: "https://github.com/openssl/openssl/releases/download/openssl-${OPENSSL_VERSION}/openssl-${OPENSSL_VERSION}.tar.gz"
    sha256: ${OPENSSL_SHA}
patches:
  ${OPENSSL_VERSION}:
    - patch_file: "x509-md5-names.patch"
      patch_description: "Android ships MD5 hash named certificates which OpenSSL no longer supports by default."
      patch_type: portability
      patch_source: https://github.com/openssl/openssl/issues/13565
EOF

cat <<EOF > x509-md5-names.patch
diff --git a/crypto/x509/by_dir.c b/crypto/x509/by_dir.c
index 1d401d0420..49a32d694f 100644
--- a/crypto/x509/by_dir.c
+++ b/crypto/x509/by_dir.c
@@ -257,9 +257,13 @@ static int get_cert_by_subject_ex(X509_LOOKUP *xl, X509_LOOKUP_TYPE type,
     }

     ctx = (BY_DIR *)xl->method_data;
+#ifdef FORCE_MD5_X509_NAME_HASHES
+    h = X509_NAME_hash_old(name);
+#else
     h = X509_NAME_hash_ex(name, libctx, propq, &i);
     if (i == 0)
         goto finish;
+#endif
     for (i = 0; i < sk_BY_DIR_ENTRY_num(ctx->dirs); i++) {
         BY_DIR_ENTRY *ent;
         int idx;
EOF

conan export . --version $OPENSSL_VERSION
popd
fi

cat <<EOF > ~/.conan2/profiles/android
{% set ndk = os.getenv('NDK_ROOT') %}
{% set ndk_bin = ndk + "/toolchains/llvm/prebuilt/linux-x86_64/bin" %}
{% set api = 29 %}

[settings]
os=Android
os.api_level={{api}}
build_type=Release
arch=armv8
compiler=clang
compiler.version=18
compiler.libcxx=c++_shared
compiler.cppstd=23

[conf]
tools.android:ndk_path={{ndk}}
tools.cmake.cmaketoolchain:generator=Ninja
tools.cmake.cmake_layout:build_folder_vars=['settings.os','settings.build_type']

[buildenv]
CC={{ndk_bin}}/aarch64-linux-android{{api}}-clang
CXX={{ndk_bin}}/aarch64-linux-android{{api}}-clang++
STRIP=${LLVM_STRIP}
EOF

conan install . --profile=android --build=missing -s build_type=Release \
    -d runtime_deploy \
    --deployer-folder $build_dir/deps \
    -c "tools.cmake.cmaketoolchain:user_toolchain=[\"$QT_ROOT/android_arm64_v8a/lib/cmake/Qt6/qt.toolchain.cmake\"]" \
    -o 'libcurl*:with_ca_path=/system/etc/security/cacerts'

cmake --preset android
cmake --build $build_dir -j