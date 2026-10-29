#!/bin/sh

# Arena Battles: packages are named "ArenaBattles-<tag or branch>-<commit>" and the app is "VCMI Arena Battles",
# so it installs next to (not over) an official VCMI install.

echo "Using Github environment variables!"
TMP_JOBID="$GITHUB_RUN_ID"
TMP_PRID="$PULL_REQUEST"
TMP_COMMIT=$(git rev-parse --short "$GITHUB_SHA")

case "$GITHUB_REF" in
	refs/tags/*) TMP_REF=${GITHUB_REF#refs/tags/} ; IS_TAG=1 ;;
	*)           TMP_REF=$(basename ${GITHUB_REF#refs/heads/}) ; IS_TAG=0 ;;
esac

VCMI_PACKAGE_BUILD="${TMP_COMMIT}"
VCMI_PACKAGE_NAME_SUFFIX="Arena Battles"
VCMI_PACKAGE_GOLDMASTER="OFF"
if [ -z "$TMP_PRID" ] || [ "$TMP_PRID" = "false" ]
then
	ref_name=$(echo "$TMP_REF" | sed 's/[^[:alnum:].]\+/_/g')
	if [ "$IS_TAG" = "1" ]
	then
		VCMI_PACKAGE_FILE_NAME="ArenaBattles-${ref_name}"
	else
		VCMI_PACKAGE_FILE_NAME="ArenaBattles-${ref_name}-${TMP_COMMIT}"
	fi
else
	VCMI_PACKAGE_FILE_NAME="ArenaBattles-PR-${TMP_PRID}-${TMP_COMMIT}"
fi

echo "VCMI_PACKAGE_FILE_NAME=${VCMI_PACKAGE_FILE_NAME}"
echo "VCMI_PACKAGE_BUILD=${VCMI_PACKAGE_BUILD}"
echo "VCMI_PACKAGE_NAME_SUFFIX=${VCMI_PACKAGE_NAME_SUFFIX}"
echo "VCMI_PACKAGE_GOLDMASTER=${VCMI_PACKAGE_GOLDMASTER}"

export VCMI_PACKAGE_FILE_NAME
export VCMI_PACKAGE_BUILD
export VCMI_PACKAGE_NAME_SUFFIX
export VCMI_PACKAGE_GOLDMASTER
