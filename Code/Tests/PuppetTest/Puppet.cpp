#include "Puppet.h"

#include "../../XML/tinyxml.h"
#include "../../Assets.h"
#include "../../Graphics/Sprite.h"
#include "../../Monocle.h"

namespace Monocle
{
	Part::Part(int id, const std::string &name, const std::string &imageFilename)
		: Entity(), id(id), name(name), sprite(NULL)
	{
		sprite = new Sprite(imageFilename);
		SetGraphic(sprite);
	}

	Part::Part()
		: Entity(), id(-1), sprite(NULL)
	{

	}

	int Part::GetID()
	{
		return id;
	}

	bool Part::IsName(const std::string &name)
	{
		return this->name == name;
	}

	bool Part::IsID(int id)
	{
		return this->id == id;
	}

	void Part::Save(FileNode *fileNode)
	{
		fileNode->Write("id", id);
		fileNode->Write("name", name);
		fileNode->Write("image", sprite->texture->GetName());
	}

	void Part::Load(FileNode *fileNode)
	{
		fileNode->Read("id", id);
		fileNode->Read("name", name);
		std::string image;
		fileNode->Read("image", image);
		SetGraphic(NULL);
		sprite = new Sprite(image);
		SetGraphic(sprite);
	}

	KeyFrame::KeyFrame()
		: Transform(), easeType(EASE_LINEAR)
	{
	}

	inline float KeyFrame::GetTime()
	{
		return time;
	}

	void KeyFrame::SetTime(float time)
	{
		this->time = time;
	}

	void KeyFrame::Save(FileNode *fileNode)
	{
		Transform::Save(fileNode);

		fileNode->Write("time", time);
	}

	void KeyFrame::Load(FileNode *fileNode)
	{
		Transform::Load(fileNode);

		fileNode->Read("time", time);
	}

	PartKeyFrames::PartKeyFrames()
		: part(NULL)
	{
	}

	void PartKeyFrames::GetKeyframeForTime(float time, KeyFrame **prev, KeyFrame **next)
	{
		// go through all the keyframes, check time
		KeyFrame *lastKeyFrame = NULL;
		for (std::list<KeyFrame>::iterator i = keyFrames.begin(); i != keyFrames.end(); ++i)
		{
			KeyFrame *keyFrame = &(*i);
			if (time > keyFrame->GetTime())
			{
				*prev = keyFrame;
			}
			else
			{
				*next = keyFrame;
				return;
			}
			lastKeyFrame = keyFrame;
		}
	}

	void PartKeyFrames::SetPart(Part *part)
	{
		this->part = part;
	}

	inline Part* PartKeyFrames::GetPart()
	{
		return part;
	}

	void PartKeyFrames::AddKeyFrame(const KeyFrame &keyFrame)
	{
		keyFrames.push_back(keyFrame);
	}

	KeyFrame *PartKeyFrames::GetLastKeyFrame()
	{
		if (keyFrames.size() == 0)
			return NULL;
		return &keyFrames.back();
	}


	Animation::Animation()
		: time(0.0f), length(0.0)
	{
	}

	void Animation::Update()
	{
		time += Monocle::deltaTime;

		if (time > length)
		{
			time = 0.0f;
		}

		for (std::list<PartKeyFrames>::iterator i = partKeyFrames.begin(); i != partKeyFrames.end(); ++i)
		{
			PartKeyFrames *currentPartKeyFrames = &(*i);
			if (currentPartKeyFrames)
			{
				KeyFrame *prev=NULL, *next=NULL;
				currentPartKeyFrames->GetKeyframeForTime(time, &prev, &next);
				if (prev && !next)
				{
					currentPartKeyFrames->GetPart()->LerpTransform(prev, prev, 1.0f);
				}
				else if (prev && next)
				{
					float diff = next->GetTime() - prev->GetTime();
					float p = (time - prev->GetTime()) / diff;

					//printf("LerpTransform %f\n", p);
					// adjust p by ease
					currentPartKeyFrames->GetPart()->LerpTransform(prev, next, Tween::Ease(p, EASE_INOUTSIN));//prev->easeType));
				}
			}
		}
	}

	std::string Animation::GetName()
	{
		return name;
	}

	bool Animation::IsName(const std::string &name)
	{
		return (this->name == name);
	}

	void Animation::AddPartKeyFrames(const PartKeyFrames &partKeyFrames)
	{
		this->partKeyFrames.push_back(partKeyFrames);
	}
	
	void Animation::SetLength(float length)
	{
		this->length = length;
	}

	void Animation::RefreshLength()
	{
		length = -1.0f;
		for (std::list<PartKeyFrames>::iterator i = partKeyFrames.begin(); i != partKeyFrames.end(); ++i)
		{
			KeyFrame *keyFrame = (*i).GetLastKeyFrame();
			if (keyFrame)
			{
				float t = keyFrame->GetTime();
				if (t > length)
					length = t;
			}
		}
	}

	void Animation::Save(FileNode *fileNode)
	{
		fileNode->Write("name", name);
	}

	void Animation::Load(FileNode *fileNode)
	{
		fileNode->Read("name", name);
	}

	Puppet::Puppet()
		: isPlaying(false), isPaused(false)
	{
	}

	void Puppet::Load(const std::string &filename, Entity *entity)
	{
		animations.clear();
		// delete parts?
		parts.clear();

		TiXmlDocument xmlDoc(Assets::GetContentPath() + filename);
		
		if (xmlDoc.LoadFile())
		{
			/// Parts
			TiXmlElement *xmlParts = xmlDoc.FirstChildElement("Parts");
			if (xmlParts)
			{
				LoadParts(xmlParts, entity);
			}

			/// Animations
			TiXmlElement *xmlAnimations = xmlDoc.FirstChildElement("Animations");
			if (xmlAnimations)
			{
				/// Animation
				TiXmlElement *xmlAnimation = xmlAnimations->FirstChildElement("Animation");
				while (xmlAnimation)
				{
					XMLFileNode xmlFileNode(xmlAnimation);

					Animation animation;
					animation.Load(&xmlFileNode);

					/// PartKeyFrames
					TiXmlElement *xmlPartKeyFrames = xmlAnimation->FirstChildElement("PartKeyFrames");
					while (xmlPartKeyFrames)
					{
						PartKeyFrames partKeyFrames;
						int id = -1;
						if (xmlPartKeyFrames->Attribute("id"))
							id = atoi(xmlPartKeyFrames->Attribute("id"));
						partKeyFrames.SetPart(GetPartByID(id));

						/// KeyFrame
						TiXmlElement *xmlKeyFrame = xmlPartKeyFrames->FirstChildElement("KeyFrame");
						while (xmlKeyFrame)
						{
							XMLFileNode xmlFileNode2(xmlKeyFrame);

							KeyFrame keyFrame;
							keyFrame.Load(&xmlFileNode2);
							partKeyFrames.AddKeyFrame(keyFrame);

							xmlKeyFrame = xmlKeyFrame->NextSiblingElement("KeyFrame");
						}

						animation.AddPartKeyFrames(partKeyFrames);

						xmlPartKeyFrames = xmlPartKeyFrames->NextSiblingElement("PartKeyFrames");
					}

					animation.RefreshLength();
					animations.push_back(animation);

					xmlAnimation = xmlAnimation->NextSiblingElement("Animation");
				}
			}
		}
	}

	void Puppet::LoadParts(TiXmlElement *element, Entity *intoEntity)
	{
		TiXmlElement *xmlPart = element->FirstChildElement("Part");
		while (xmlPart)
		{
			XMLFileNode xmlFileNode(xmlPart);

			Part *part = new Part();
			part->Load(&xmlFileNode);
			
			LoadParts(xmlPart, part);

			parts.push_back(part);

			/// TODO...?
			intoEntity->Add(part);

			xmlPart = xmlPart->NextSiblingElement("Part");
		}
	}

	void Puppet::Play(const std::string &animationName, bool isLooping)
	{
		currentAnimation = GetAnimationByName(animationName);

		if (currentAnimation)
		{
			this->isPlaying = true;
			this->isLooping = isLooping;
		}

	}

	void Puppet::Update()
	{
		if (currentAnimation)
		{
			if (isPlaying)
			{
				currentAnimation->Update();
			}
		}
	}

	void Puppet::Stop()
	{
		isPlaying = false;
		isPaused = false;
	}

	void Puppet::Pause()
	{
		isPaused = true;
	}

	void Puppet::Resume()
	{
		isPaused = false;
	}

	bool Puppet::IsPlaying()
	{
		return isPlaying;
	}

	bool Puppet::IsPaused()
	{
		return isPaused;
	}

	Animation *Puppet::GetAnimationByName(const std::string &name)
	{
		for (std::list<Animation>::iterator i = animations.begin(); i != animations.end(); ++i)
		{
			return &(*i);
		}
		return NULL;
	}

	Part *Puppet::GetPartByName(const std::string &name)
	{
		for (std::list<Part*>::iterator i = parts.begin(); i != parts.end(); ++i)
		{
			if ((*i)->IsName(name))
			{
				return *i;
			}
		}
		return NULL;
	}

	Part *Puppet::GetPartByID(int id)
	{
		for (std::list<Part*>::iterator i = parts.begin(); i != parts.end(); ++i)
		{
			if ((*i)->IsID(id))
			{
				return *i;
			}
		}
		return NULL;
	}
}