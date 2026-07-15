# SPDX-License-Identifier: GPL-2.0-or-later
#
# Builder image, run with --platform linux/arm64 (see build.sh) so it's
# genuinely aarch64 -- gcc here is aarch64-native, not a cross-compiler.

ARG ALPINE_BRANCH=latest
FROM alpine:${ALPINE_BRANCH}
RUN apk add --no-cache alpine-sdk
