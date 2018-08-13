#pragma once
#include "exec_elf.h"
#include <QVector>
#include <QJsonDocument>
#include <QFile>

class ElfBuilder
{
private:
	QString sopath_;
	QFile sofile_;

	QJsonDocument json_doc;
	QFile json_file_;
	QString json_;	//json文件名称

	Elf32_Addr load_bias_;
	QVector<Elf32_Phdr> phdrs_;
	QStringList phdr_datapaths;
	QVector<Elf32_Dyn> dyns_;

	Elf32_Ehdr ehdr_;
	Elf32_Phdr ph_myload_;
	Elf32_Phdr ph_phdr_;
	Elf32_Phdr ph_dynamic_;

public:
	ElfBuilder(QString json);
	~ElfBuilder();

	//从json文件中读取so的必要信息
	bool ReadJson();

	//根据json中的信息重组so文件
	bool BuildInfo();

	//将数据写入, 得到so文件
	bool Write();

	bool Build();
};
