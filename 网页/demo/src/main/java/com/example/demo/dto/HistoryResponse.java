package com.example.demo.dto;

import java.time.LocalDateTime;
import java.util.List;

public record HistoryResponse(
		LocalDateTime start,
		LocalDateTime end,
		long count,
		List<HistoryPointResponse> records
) {
}
