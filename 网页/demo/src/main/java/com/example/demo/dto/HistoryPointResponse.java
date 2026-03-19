package com.example.demo.dto;

import java.time.LocalDateTime;

public record HistoryPointResponse(
		LocalDateTime createdAt,
		String source,
		Double temperature,
		Double humidity,
		Double soil,
		Integer light,
		Integer co2,
		Double brightness,
		Double greenRatio,
		Boolean motionFlag,
		Boolean visionOk
) {
}
