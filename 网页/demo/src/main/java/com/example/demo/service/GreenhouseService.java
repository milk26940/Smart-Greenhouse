package com.example.demo.service;

import com.example.demo.dto.HealthResponse;
import com.example.demo.dto.HistoryPointResponse;
import com.example.demo.dto.HistoryResponse;
import com.example.demo.dto.MetricStatsResponse;
import com.example.demo.dto.PresenceResponse;
import com.example.demo.dto.RealtimeResponse;
import com.example.demo.dto.StatsResponse;
import com.example.demo.dto.TelemetryUpsertRequest;
import com.example.demo.model.TelemetryRecord;
import com.example.demo.repository.TelemetryRecordRepository;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.Duration;
import java.time.LocalDateTime;
import java.util.DoubleSummaryStatistics;
import java.util.List;
import java.util.Optional;

@Service
public class GreenhouseService {

	private final TelemetryRecordRepository repository;

	public GreenhouseService(TelemetryRecordRepository repository) {
		this.repository = repository;
	}

	public HealthResponse health() {
		return new HealthResponse("online", "mysql", "greenhouse-web");
	}

	public Optional<RealtimeResponse> realtime() {
		return repository.findTopByOrderByCreatedAtDesc().map(this::toRealtimeResponse);
	}

	public HistoryResponse history(int hours) {
		LocalDateTime end = LocalDateTime.now();
		LocalDateTime start = end.minusHours(hours);
		List<HistoryPointResponse> records = repository.findByCreatedAtBetweenOrderByCreatedAtAsc(start, end)
				.stream()
				.map(this::toHistoryPoint)
				.toList();
		return new HistoryResponse(start, end, records.size(), records);
	}

	public StatsResponse stats(int hours) {
		LocalDateTime end = LocalDateTime.now();
		LocalDateTime start = end.minusHours(hours);
		List<TelemetryRecord> records = repository.findByCreatedAtBetweenOrderByCreatedAtAsc(start, end);
		if (records.isEmpty()) {
			MetricStatsResponse empty = new MetricStatsResponse(0, 0, 0, 0);
			return new StatsResponse(start, end, 0, empty, empty, empty, empty, empty);
		}

		TelemetryRecord latest = records.get(records.size() - 1);
		return new StatsResponse(
				start,
				end,
				records.size(),
				buildMetricStats(records.stream().mapToDouble(TelemetryRecord::getTemperature).summaryStatistics(), latest.getTemperature()),
				buildMetricStats(records.stream().mapToDouble(TelemetryRecord::getHumidity).summaryStatistics(), latest.getHumidity()),
				buildMetricStats(records.stream().mapToDouble(TelemetryRecord::getSoil).summaryStatistics(), latest.getSoil()),
				buildMetricStats(records.stream().mapToDouble(TelemetryRecord::getLight).summaryStatistics(), latest.getLight().doubleValue()),
				buildMetricStats(records.stream().mapToDouble(TelemetryRecord::getCo2).summaryStatistics(), latest.getCo2().doubleValue())
		);
	}

	public PresenceResponse presence() {
		Optional<TelemetryRecord> latest = repository.findTopByOrderByCreatedAtDesc();
		if (latest.isEmpty()) {
			return new PresenceResponse("UNKNOWN", "UNKNOWN", "UNKNOWN", "UNKNOWN", "UNKNOWN", null);
		}

		TelemetryRecord record = latest.get();
		boolean online = Duration.between(record.getCreatedAt(), LocalDateTime.now()).getSeconds() <= 30;
		String sensor1 = online ? "ONLINE" : "OFFLINE";
		return new PresenceResponse(sensor1, "UNKNOWN", "UNKNOWN", "UNKNOWN", "UNKNOWN", record.getCreatedAt());
	}

	@Transactional
	public RealtimeResponse ingest(TelemetryUpsertRequest request) {
		TelemetryRecord record = new TelemetryRecord();
		record.setNodeId(request.nodeId() != null ? request.nodeId() : 1);
		record.setCreatedAt(request.createdAt() != null ? request.createdAt() : LocalDateTime.now());
		record.setTemperature(requireDouble(request.temperature(), "temperature"));
		record.setHumidity(requireDouble(request.humidity(), "humidity"));
		record.setSoil(requireDouble(request.soil(), "soil"));
		record.setLight(requireInteger(request.light(), "light"));
		record.setCo2(requireInteger(request.co2(), "co2"));
		record.setBrightness(requireDouble(request.brightness(), "brightness"));
		record.setGreenRatio(requireDouble(request.greenRatio(), "greenRatio"));
		record.setMotionFlag(requireBoolean(request.motionFlag(), "motionFlag"));
		record.setVisionOk(requireBoolean(request.visionOk(), "visionOk"));
		record.setSource(request.source() != null ? request.source() : "manual");
		return toRealtimeResponse(repository.save(record));
	}

	private RealtimeResponse toRealtimeResponse(TelemetryRecord record) {
		String fan = record.getTemperature() > 35 ? "ON" : "OFF";
		String pump = record.getSoil() < 40 ? "ON" : "OFF";
		String led = (record.getLight() < 1200 && record.getBrightness() < 60) ? "ON" : "OFF";
		String summary = pump.equals("ON") ? "土壤偏干，建议启动灌溉" : "环境稳定，当前无需额外控制";
		return new RealtimeResponse(
				true,
				summary,
				record.getSource(),
				record.getCreatedAt(),
				new RealtimeResponse.EnvPayload(
						record.getTemperature(),
						record.getHumidity(),
						record.getSoil(),
						record.getLight(),
						record.getCo2()
				),
				new RealtimeResponse.DecisionPayload(fan, pump, led, summary),
				new RealtimeResponse.VisionPayload(
						record.getBrightness(),
						record.getGreenRatio(),
						record.getMotionFlag(),
						record.getVisionOk()
				),
				new RealtimeResponse.PresencePayload(
						"ONLINE",
						"UNKNOWN",
						"UNKNOWN",
						"UNKNOWN",
						"UNKNOWN"
				)
		);
	}

	private HistoryPointResponse toHistoryPoint(TelemetryRecord record) {
		return new HistoryPointResponse(
				record.getCreatedAt(),
				record.getSource(),
				record.getTemperature(),
				record.getHumidity(),
				record.getSoil(),
				record.getLight(),
				record.getCo2(),
				record.getBrightness(),
				record.getGreenRatio(),
				record.getMotionFlag(),
				record.getVisionOk()
		);
	}

	private MetricStatsResponse buildMetricStats(DoubleSummaryStatistics statistics, double latest) {
		return new MetricStatsResponse(
				round(statistics.getAverage()),
				round(statistics.getMax()),
				round(statistics.getMin()),
				round(latest)
		);
	}

	private double requireDouble(Double value, String fieldName) {
		if (value == null) {
			throw new IllegalArgumentException(fieldName + " is required");
		}
		return value;
	}

	private int requireInteger(Integer value, String fieldName) {
		if (value == null) {
			throw new IllegalArgumentException(fieldName + " is required");
		}
		return value;
	}

	private boolean requireBoolean(Boolean value, String fieldName) {
		if (value == null) {
			throw new IllegalArgumentException(fieldName + " is required");
		}
		return value;
	}

	private double round(double value) {
		return Math.round(value * 10.0) / 10.0;
	}
}
