// SPDX-FileCopyrightText: 2026 DeckardDetribine and the ReSkateMusicPacker contributors
// SPDX-FileCopyrightText: 2026 the ReSkate contributors
// SPDX-License-Identifier: GPL-3.0-only
// Ported from the ReSkate project (https://github.com/Dingo-Shenanigans/ReSkate); see NOTICE.md.
#pragma once

#include "ebx_document.h"

#include <cstdint>
#include <vector>

namespace dingosdk::frostbite::ebx {

[[nodiscard]] std::uint16_t ensure_fixup_type(Document& document, std::int32_t descriptor);

[[nodiscard]] std::vector<std::byte> write_document(const Document& document);

}
