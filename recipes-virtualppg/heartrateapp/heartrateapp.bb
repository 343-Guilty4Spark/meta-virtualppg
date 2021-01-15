DESCRIPTION = "Heart Rate Application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"
SRC_URI = "file://heartrateapp.c"

S = "${WORKDIR}"

do_compile() {
  set CFLAGS -g
  LDLIBS="-lm -lpthread"
  ${CC} ${CFLAGS} heartrateapp.c ${LDFLAGS} -o HeartRateApp ${LDLIBS}
  unset CFLAGS
}

do_install() {
  install -d ${D}${bindir}
  install -m 0755 HeartRateApp ${D}${bindir}
}
