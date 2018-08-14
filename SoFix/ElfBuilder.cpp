#include "ElfBuilder.h"
#include <QJsonParseError>
#include <QJsonObject>
#include <QDebug>
#include <QJsonArray>
#include "Util.h"

#define QSTR8BIT(s) (QString::fromLocal8Bit(s))

ElfBuilder::ElfBuilder(QString json)
	: json_(json), load_bias_(0)
{
	json_file_.setFileName(json);
}

ElfBuilder::~ElfBuilder()
{

}

bool ElfBuilder::ReadJson()
{
	if (!json_file_.open(QIODevice::ReadOnly | QIODevice::ExistingOnly))
	{
		qDebug() << "could't open json config";
		return false;
	}

	QByteArray json_data = json_file_.readAll();
	json_file_.close();

	QJsonParseError json_error;
	QJsonDocument jsonDoc(QJsonDocument::fromJson(json_data, &json_error));

	if (json_error.error != QJsonParseError::NoError || jsonDoc.isNull())
	{
		qDebug() << "json error or null!";
		return false;
	}

	QJsonObject rootObj = jsonDoc.object();

	//因为是预先定义好的JSON数据格式，所以这里可以这样读取
	if (rootObj.contains("file name"))
	{
		sopath_ = rootObj.value("file name").toString();
	}

	if (rootObj.contains("load_bias"))
	{
		load_bias_ = rootObj.value("load_bias").toString("0").toUInt(nullptr, 16);
	}

	if (rootObj.contains("program headers"))
	{
		QJsonArray subArray = rootObj.value("program headers").toArray();
		for (int i = 0; i < subArray.size(); i++)
		{
			QJsonObject obj = subArray.at(i).toObject();
			Elf32_Phdr phdr = { 0 };
			QString raw_path;

			if (obj.contains("p_type"))
			{
				phdr.p_type = obj.value("p_type").toString("0").toUInt(nullptr, 16);
				if (phdr.p_type == PT_NULL || phdr.p_type == PT_DYNAMIC || phdr.p_type == PT_PHDR)
				{
					//过滤掉无效项PT_NULL以及PT_DYNAMIC, PT_PHDR, 一会生成它们
					continue;
				}
			}
			if (obj.contains("p_offset"))
			{
				phdr.p_offset = obj.value("p_offset").toString("0").toUInt(nullptr, 16);
			}
			if (obj.contains("p_vaddr"))
			{
				phdr.p_vaddr = obj.value("p_vaddr").toString("0").toUInt(nullptr, 16) - load_bias_;
			}
			if (obj.contains("p_paddr"))
			{
				phdr.p_paddr = obj.value("p_paddr").toString("0").toUInt(nullptr, 16) - load_bias_;
			}
			if (obj.contains("p_filesz"))
			{
				phdr.p_filesz = obj.value("p_filesz").toString("0").toUInt(nullptr, 16);
			}
			if (obj.contains("p_memsz"))
			{
				phdr.p_memsz = obj.value("p_memsz").toString("0").toUInt(nullptr, 16);
			}
			if (obj.contains("p_flags"))
			{
				phdr.p_flags = obj.value("p_flags").toString("0").toUInt(nullptr, 16);
			}
			if (obj.contains("p_align"))
			{
				phdr.p_align = obj.value("p_align").toString("0").toUInt(nullptr, 16);
			}
			if (obj.contains("raw_file"))
			{
				raw_path = obj.value("raw_file").toString("");
			}

			phdr_datapaths.push_back(raw_path);
			phdrs_.push_back(phdr);
		}
	}

	if (rootObj.contains("dynamic section"))
	{
		QJsonObject obj = rootObj.value("dynamic section").toObject();
		if (!obj.contains("DT_HASH") || !obj.contains("DT_STRTAB") || !obj.contains("DT_SYMTAB"))
		{
			qDebug() << QSTR8BIT("不存在DT_HASH, DT_STRTAB, DT_SYMTAB, 无法继续...");
			return false;
		}

		Elf32_Addr strtab = 0;

		if (obj.contains("DT_HASH"))
		{
			Elf32_Dyn dyn = { 0 };
			dyn.d_tag = 0x00000004;
			dyn.d_un.d_ptr = obj.value("DT_HASH").toString("0").toUInt(nullptr, 16) - load_bias_;
			dyns_.push_back(dyn);
		}
		if (obj.contains("DT_STRTAB"))
		{
			Elf32_Dyn dyn = { 0 };
			dyn.d_tag = 0x00000005;
			dyn.d_un.d_ptr = obj.value("DT_STRTAB").toString("0").toUInt(nullptr, 16) - load_bias_;
			dyns_.push_back(dyn);
			strtab = dyn.d_un.d_ptr;
		}
		if (obj.contains("DT_STRSZ") && !obj.value("DT_STRSZ").isNull())
		{
			Elf32_Dyn dyn = { 0 };
			dyn.d_tag = 0x0000000a;
			dyn.d_un.d_val = obj.value("DT_STRSZ").toString("0").toUInt(nullptr, 16);
			dyns_.push_back(dyn);
		}
		if (obj.contains("DT_SYMTAB"))
		{
			Elf32_Dyn dyn = { 0 };
			dyn.d_tag = 0x00000006;
			dyn.d_un.d_ptr = obj.value("DT_SYMTAB").toString("0").toUInt(nullptr, 16) - load_bias_;
			dyns_.push_back(dyn);
		}
		if (obj.contains("DT_JMPREL") && !obj.value("DT_JMPREL").isNull())
		{
			Elf32_Dyn dyn = { 0 };
			dyn.d_tag = 0x00000017;
			dyn.d_un.d_ptr = obj.value("DT_JMPREL").toString("0").toUInt(nullptr, 16) - load_bias_;
			dyns_.push_back(dyn);
		}
		if (obj.contains("DT_PLTRELSZ") && !obj.value("DT_PLTRELSZ").isNull())
		{
			Elf32_Dyn dyn = { 0 };
			dyn.d_tag = 0x00000002;
			dyn.d_un.d_val = obj.value("DT_PLTRELSZ").toString("0").toUInt(nullptr, 16);
			dyns_.push_back(dyn);
		}
		if (obj.contains("DT_REL") && !obj.value("DT_REL").isNull())
		{
			Elf32_Dyn dyn = { 0 };
			dyn.d_tag = 0x00000011;
			dyn.d_un.d_ptr = obj.value("DT_REL").toString("0").toUInt(nullptr, 16) - load_bias_;
			dyns_.push_back(dyn);
		}
		if (obj.contains("DT_RELSZ") && !obj.value("DT_RELSZ").isNull())
		{
			Elf32_Dyn dyn = { 0 };
			dyn.d_tag = 0x00000012;
			dyn.d_un.d_val = obj.value("DT_RELSZ").toString("0").toUInt(nullptr, 16);
			dyns_.push_back(dyn);
		}
		if (obj.contains("DT_INIT") && !obj.value("DT_INIT").isNull())
		{
			Elf32_Dyn dyn = { 0 };
			dyn.d_tag = 0x0000000C;
			dyn.d_un.d_ptr = obj.value("DT_INIT").toString("0").toUInt(nullptr, 16) - load_bias_;
			dyns_.push_back(dyn);
		}
		if (obj.contains("DT_FINI") && !obj.value("DT_FINI").isNull())
		{
			Elf32_Dyn dyn = { 0 };
			dyn.d_tag = 0x0000000D;
			dyn.d_un.d_ptr = obj.value("DT_FINI").toString("0").toUInt(nullptr, 16) - load_bias_;
			dyns_.push_back(dyn);
		}
		if (obj.contains("DT_INIT_ARRAY") && !obj.value("DT_INIT_ARRAY").isNull())
		{
			Elf32_Dyn dyn = { 0 };
			dyn.d_tag = 0x00000019;
			dyn.d_un.d_ptr = obj.value("DT_INIT_ARRAY").toString("0").toUInt(nullptr, 16) - load_bias_;
			dyns_.push_back(dyn);
		}
		if (obj.contains("DT_INIT_ARRAYSZ") && !obj.value("DT_INIT_ARRAYSZ").isNull())
		{
			Elf32_Dyn dyn = { 0 };
			dyn.d_tag = 0x0000001b;
			dyn.d_un.d_val = obj.value("DT_INIT_ARRAYSZ").toString("0").toUInt(nullptr, 16);
			dyns_.push_back(dyn);
		}
		if (obj.contains("DT_FINI_ARRAY") && !obj.value("DT_FINI_ARRAY").isNull())
		{
			Elf32_Dyn dyn = { 0 };
			dyn.d_tag = 0x0000001a;
			dyn.d_un.d_ptr = obj.value("DT_FINI_ARRAY").toString("0").toUInt(nullptr, 16) - load_bias_;
			dyns_.push_back(dyn);
		}
		if (obj.contains("DT_FINI_ARRAYSZ") && !obj.value("DT_FINI_ARRAYSZ").isNull())
		{
			Elf32_Dyn dyn = { 0 };
			dyn.d_tag = 0x0000001c;
			dyn.d_un.d_val = obj.value("DT_FINI_ARRAYSZ").toString("0").toUInt(nullptr, 16);
			dyns_.push_back(dyn);
		}

		if (obj.contains("DT_NEEDED") && !obj.value("DT_NEEDED").isNull())
		{
			QJsonArray subArray = obj.value("DT_NEEDED").toArray();
			for (int i = 0; i < subArray.size(); i++)
			{
				Elf32_Dyn dyn = { 0 };
				dyn.d_tag = 0x00000001;
				dyn.d_un.d_ptr = subArray.at(i).toString("0").toUInt(nullptr, 16) - load_bias_ - strtab;
				dyns_.push_back(dyn);
			}
		}
	}

	if (rootObj.contains("options"))
	{
		QJsonArray subArray = rootObj.value("options").toArray();
		for (int i = 0; i < subArray.size(); i++)
		{
			QJsonObject obj = subArray.at(i).toObject();
			Option op = { 0 };

			if (obj.contains("offset"))
			{
				op.offset = obj.value("offset").toString("0").toUInt(nullptr, 16);
			}
			if (obj.contains("item_size"))
			{
				op.item_size = obj.value("item_size").toString("0").toUInt(nullptr, 16);
			}
			if (obj.contains("count"))
			{
				op.count = obj.value("count").toString("0").toUInt(nullptr, 16);
			}
			if (obj.contains("bias"))
			{
				op.bias = obj.value("bias").toString("0").toInt(nullptr, 16);
			}

			options_.push_back(op);
		}
	}

	return true;
}

#define E_IDENT ("\x7f\x45\x4c\x46\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00")

bool ElfBuilder::BuildInfo()
{
	//整个文件的组成: 
	//Ehdr | Phdr | .dynamic | LOAD段1 | LOAD段2 | ...

	//构造ehdr
	memcpy(ehdr_.e_ident, E_IDENT, ELF_NIDENT);
	ehdr_.e_type = ET_DYN;
	ehdr_.e_machine = EM_ARM;
	ehdr_.e_version = EV_CURRENT;
	ehdr_.e_phoff = sizeof(Elf32_Ehdr);
	ehdr_.e_shoff = 0;	//非必需字段
	ehdr_.e_flags = 0x5000000;
	ehdr_.e_ehsize = sizeof(Elf32_Ehdr);
	ehdr_.e_phentsize = sizeof(Elf32_Phdr);
	ehdr_.e_phnum = phdrs_.length() + 3;	//自己生成PT_LOAD, PT_PHDR, PT_DYNAMIC
	ehdr_.e_shentsize = sizeof(Elf32_Shdr);
	ehdr_.e_shnum = 0;	//非必需字段
	ehdr_.e_shstrndx = 0;	//非必需字段

	//获取LOAD段的最小最大地址
	bool found_pt_load = false;
	Elf32_Addr min_vaddr = 0xFFFFFFFFU;
	Elf32_Addr max_vaddr = 0x00000000U;

	for (Elf32_Phdr &ph : phdrs_)
	{	
		if (ph.p_type != PT_LOAD)
		{
			continue;
		}
		found_pt_load = true;
		if (ph.p_vaddr < min_vaddr)
		{
			min_vaddr = ph.p_vaddr;
		}
		if (ph.p_vaddr + ph.p_memsz > max_vaddr)
		{
			max_vaddr = ph.p_vaddr + ph.p_memsz;
		}
	}

	if (!found_pt_load)
	{
		qDebug() << QSTR8BIT("未找到LOAD段, 玩不下去了...");
		return false;
	}
	min_vaddr = PAGE_START(min_vaddr);
	max_vaddr = PAGE_END(max_vaddr);

	//构造一个新的load段, 来存放我们的PT_PHDR, .dynamic节数据
	ph_myload_.p_type = PT_LOAD;
	ph_myload_.p_offset = 0;
	ph_myload_.p_vaddr = max_vaddr;	//将我们的可加载段数据映射到原load段之后
	ph_myload_.p_paddr = max_vaddr;
	ph_myload_.p_filesz = ehdr_.e_phoff + (phdrs_.length() + 3) * sizeof(Elf32_Phdr) + (dyns_.length() + 1) * sizeof(Elf32_Dyn);
	ph_myload_.p_memsz = ehdr_.e_phoff + (phdrs_.length() + 3) * sizeof(Elf32_Phdr) + (dyns_.length() + 1) * sizeof(Elf32_Dyn);
	ph_myload_.p_flags = PF_R;
	ph_myload_.p_align = 0x1000;

	//构造PT_PHDR
	ph_phdr_.p_type = PT_PHDR;
	ph_phdr_.p_offset = ehdr_.e_phoff;
	ph_phdr_.p_vaddr = ph_myload_.p_vaddr + ehdr_.e_phoff;	//PT_PHDR放在我们自己的可加载段中
	ph_phdr_.p_paddr = ph_myload_.p_paddr + ehdr_.e_phoff;
	ph_phdr_.p_filesz = (phdrs_.length() + 3) * sizeof(Elf32_Phdr);
	ph_phdr_.p_memsz = (phdrs_.length() + 3) * sizeof(Elf32_Phdr);
	ph_phdr_.p_flags = PF_R;
	ph_phdr_.p_align = 0x4;

	//构造PT_DYNAMIC, 文件数据放在整个程序头表之后, 内存映射到整个程序头表之后
	ph_dynamic_.p_type = PT_DYNAMIC;
	ph_dynamic_.p_offset = ehdr_.e_phoff + (phdrs_.length() + 3) * sizeof(Elf32_Phdr);
	ph_dynamic_.p_vaddr = ph_myload_.p_vaddr + ehdr_.e_phoff + (phdrs_.length() + 3) * sizeof(Elf32_Phdr);
	ph_dynamic_.p_paddr = ph_myload_.p_paddr + ehdr_.e_phoff + (phdrs_.length() + 3) * sizeof(Elf32_Phdr);
	ph_dynamic_.p_filesz = (dyns_.length() + 1) * sizeof(Elf32_Dyn);	//这里的+1是因为最后需要一个DT_NULL段表示.dynamic结束
	ph_dynamic_.p_memsz = (dyns_.length() + 1) * sizeof(Elf32_Dyn);
	ph_dynamic_.p_flags = PF_R | PF_W;
	ph_dynamic_.p_align = 0x4;

	//将原来的LOAD段的文件偏移向后移动 PAGE_END(ph_myload_.p_offset+ph_myload_.p_filesz), 确保数据不被覆盖
	for (Elf32_Phdr &ph : phdrs_)
	{
		if (ph.p_type != PT_LOAD)
		{
			continue;
		}

		ph.p_offset += PAGE_END(ph_myload_.p_offset + ph_myload_.p_filesz);
	}

	//到此, so的必需数据已经准备就绪
	return true;
}

bool ElfBuilder::Write()
{
	sofile_.setFileName(sopath_);
	if (!sofile_.open(QIODevice::ReadWrite | QIODevice::Truncate))
	{
		return false;
	}

	//写入Ehdr
	sofile_.seek(0);
	sofile_.write((char *)&ehdr_, sizeof(Elf32_Ehdr));

	//写入Phdr
	sofile_.seek(ehdr_.e_phoff);
	sofile_.write((char *)&ph_phdr_, sizeof(Elf32_Phdr));
	sofile_.write((char *)&ph_dynamic_, sizeof(Elf32_Phdr));
	sofile_.write((char *)&ph_myload_, sizeof(Elf32_Phdr));
	
	for (Elf32_Phdr &ph : phdrs_)
	{
		sofile_.write((char *)&ph, sizeof(Elf32_Phdr));
	}

	//写入.dynamic
	sofile_.seek(ph_dynamic_.p_offset);
	for (Elf32_Dyn &dyn : dyns_)
	{
		sofile_.write((char *)&dyn, sizeof(Elf32_Dyn));
	}
	//写入DT_NULL表示.dynamic结束
	Elf32_Dyn dyn = { 0 };
	sofile_.write((char *)&dyn, sizeof(Elf32_Dyn));

	//写入段数据
	for (int i = 0; i < phdrs_.length(); i++)
	{
		if (phdrs_[i].p_type != PT_LOAD)
		{
			continue;
		}

		QFile data_file(phdr_datapaths[i]);
		if (data_file.open(QIODevice::ReadOnly | QIODevice::ExistingOnly))
		{
			QByteArray data = data_file.readAll();

			//注意: 写入文件时按照so加载时实际写入文件的大小, 即[PAGE_START(), PAGE_END())
			//因此在提供的数据文件中也要确保如此
			sofile_.seek(PAGE_START(phdrs_[i].p_offset));
			qint64 size = PAGE_END(phdrs_[i].p_offset + phdrs_[i].p_filesz) - PAGE_START(phdrs_[i].p_offset);
			qint64 wc = sofile_.write(data, size);
			if (wc < size)	//如果不满足上面的条件则给个警告
			{
				qDebug() << QSTR8BIT("警告: 输入的段数据长度不足, "
					"应输入数据: PAGE_START(phdrs_[i].p_offset) ~ PAGE_END(phdr.p_offset + phdrs.p_filesz)");
			}
		}
	}

	//根据options_修正一些字节
	for (Option &op : options_)
	{
		sofile_.seek(op.offset);
		char *olds = new char[op.count * op.item_size];
		sofile_.read((char *)olds, op.item_size * op.count);
		char *tmp = olds;
		
		for (int i = 0; i < op.count; i++)
		{
			*(Elf32_Addr *)tmp += op.bias;
			tmp += op.item_size;
		}

		sofile_.seek(op.offset);
		sofile_.write((char *)olds, op.item_size * op.count);
		delete[] olds;
	}
	

	//到此, so的写入已经完成
	sofile_.close();

	return true;
}

bool ElfBuilder::Build()
{
	return ReadJson() && 
		BuildInfo() &&
		Write();
}
