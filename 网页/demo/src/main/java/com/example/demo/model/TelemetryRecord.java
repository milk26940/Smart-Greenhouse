package com.example.demo.model;

import jakarta.persistence.Column;
import jakarta.persistence.Entity;
import jakarta.persistence.GeneratedValue;
import jakarta.persistence.GenerationType;
import jakarta.persistence.Id;
import jakarta.persistence.Index;
import jakarta.persistence.Table;

import java.time.LocalDateTime;

@Entity
@Table(name = "telemetry_records", indexes = {
		@Index(name = "idx_telemetry_created_at", columnList = "createdAt")
})
public class TelemetryRecord {

	@Id
	@GeneratedValue(strategy = GenerationType.IDENTITY)
	private Long id;

	@Column(nullable = false)
	private Integer nodeId;

	@Column(nullable = false)
	private LocalDateTime createdAt;

	@Column(nullable = false)
	private Double temperature;

	@Column(nullable = false)
	private Double humidity;

	@Column(nullable = false)
	private Double soil;

	@Column(nullable = false)
	private Integer light;

	@Column(nullable = false)
	private Integer co2;

	@Column(nullable = false)
	private Double brightness;

	@Column(nullable = false)
	private Double greenRatio;

	@Column(nullable = false)
	private Boolean motionFlag;

	@Column(nullable = false)
	private Boolean visionOk;

	@Column(nullable = false, length = 32)
	private String source;

	public Long getId() {
		return id;
	}

	public Integer getNodeId() {
		return nodeId;
	}

	public void setNodeId(Integer nodeId) {
		this.nodeId = nodeId;
	}

	public LocalDateTime getCreatedAt() {
		return createdAt;
	}

	public void setCreatedAt(LocalDateTime createdAt) {
		this.createdAt = createdAt;
	}

	public Double getTemperature() {
		return temperature;
	}

	public void setTemperature(Double temperature) {
		this.temperature = temperature;
	}

	public Double getHumidity() {
		return humidity;
	}

	public void setHumidity(Double humidity) {
		this.humidity = humidity;
	}

	public Double getSoil() {
		return soil;
	}

	public void setSoil(Double soil) {
		this.soil = soil;
	}

	public Integer getLight() {
		return light;
	}

	public void setLight(Integer light) {
		this.light = light;
	}

	public Integer getCo2() {
		return co2;
	}

	public void setCo2(Integer co2) {
		this.co2 = co2;
	}

	public Double getBrightness() {
		return brightness;
	}

	public void setBrightness(Double brightness) {
		this.brightness = brightness;
	}

	public Double getGreenRatio() {
		return greenRatio;
	}

	public void setGreenRatio(Double greenRatio) {
		this.greenRatio = greenRatio;
	}

	public Boolean getMotionFlag() {
		return motionFlag;
	}

	public void setMotionFlag(Boolean motionFlag) {
		this.motionFlag = motionFlag;
	}

	public Boolean getVisionOk() {
		return visionOk;
	}

	public void setVisionOk(Boolean visionOk) {
		this.visionOk = visionOk;
	}

	public String getSource() {
		return source;
	}

	public void setSource(String source) {
		this.source = source;
	}
}
