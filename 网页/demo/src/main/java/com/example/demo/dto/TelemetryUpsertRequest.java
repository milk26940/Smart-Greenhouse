package com.example.demo.dto;

import com.fasterxml.jackson.annotation.JsonFormat;
import java.time.LocalDateTime;

public record TelemetryUpsertRequest(
		Integer nodeId,
		@JsonFormat(pattern = "yyyy-MM-dd HH:mm:ss", timezone = "Asia/Shanghai")
		LocalDateTime createdAt,
		Double temperature,
		Double humidity,
		Double soil,
		Integer light,
		Integer co2,
		Double brightness,
		Double greenRatio,
		Boolean motionFlag,
		Boolean visionOk,
		String source
) {
}
