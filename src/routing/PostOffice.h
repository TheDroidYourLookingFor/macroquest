/*
 * MacroQuest: The extension platform for EverQuest
 * Copyright (C) 2002-2022 MacroQuest Authors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#pragma once

#include "Routing.h"

#include <string>
#include <unordered_map>
#include <queue>
#include <memory>

namespace mq::postoffice {

using ReceiveCallback = std::function<void(ProtoMessagePtr&&)>;
using PostCallback = std::function<void(const std::string&)>;
using DropboxDropper = std::function<void(const std::string&)>;

class Mailbox
{
public:
	Mailbox(std::string localAddress, ReceiveCallback&& receive)
		: m_localAddress(localAddress)
		, m_receive(std::move(receive))
	{}

	~Mailbox() {}

	/**
	 * Gets the address of this mailbox
	 *
	 * @return the local address of this mailbox
	 */
	const std::string& GetAddress() const { return m_localAddress; }

	/**
	 * Delivers a message to this mailbox to be handled by the receive callback
	 *
	 * @param message the message to deliver
	 */
	void Deliver(PipeMessagePtr&& message) const;

	/**
	 * Process some messages that have been delivered
	 *
	 * @param howMany how many messages to process off the queue
	 */
	void Process(size_t howMany) const;

private:
	static ProtoMessagePtr Open(proto::routing::Envelope&& envelope, const MQMessageHeader& header);

	const std::string m_localAddress;
	const ReceiveCallback m_receive;

	mutable std::queue<ProtoMessagePtr> m_receiveQueue;
};

class Dropbox
{
	friend class PostOffice;

public:
	Dropbox()
		: m_valid(false)
	{}

	~Dropbox() {}

	Dropbox(std::string localAddress, PostCallback&& post, DropboxDropper&& unregister);
	Dropbox(const Dropbox& other);
	Dropbox(Dropbox&& other) noexcept;
	//Dropbox& operator=(const Dropbox& other);
	Dropbox& operator=(Dropbox other) noexcept;

	/**
	 * Sends a message to an address
	 *
	 * @tparam ID an identifier to be used by the receiver, must cast to uint32_t
	 * @tparam T the message being sent, usually some kind of proto
	 *
	 * @param address the address to send the message
	 * @param messageId a message ID used to route the message at the receiver
	 * @param obj the message (as an object)
	 */
	template <typename ID, typename T>
	void Post(const proto::routing::Address& address, ID messageId, const T& obj)
	{
		if (IsValid()) m_post(Stuff(address, messageId, obj));
	}

	/**
	 * Sends a message to an address
	 *
	 * @tparam ID an identifier to be used by the receiver, must cast to uint32_t
	 * @tparam T the message being sent, usually some kind of proto
	 *
	 * @param address the address to send the message
	 * @param messageId a message ID used to route the message at the receiver
	 * @param obj the message (as a data string)
	 */
	template <typename ID>
	inline void Post(const proto::routing::Address& address, ID messageId, const std::string& obj)
	{
		if (IsValid()) m_post(StuffData(address, messageId, obj));
	}

	/**
	 * Sends an empty message to an address
	 *
	 * @tparam ID an identifier to be used by the receiver, must cast to uint32_t
	 *
	 * @param address the address to send the message
	 * @param messageId a message ID used to route the message at the receiver
	 */
	template <typename ID>
	void Post(const proto::routing::Address& address, ID messageId)
	{
		if (IsValid()) m_post(StuffData(address, messageId, ""));
	}

	/**
	 * Sends a reply to the sender of a message
	 *
	 * @tparam ID an identifier to be used by the receiver, must cast to uint32_t
	 * @tparam T the message being sent, usually some kind of proto
	 *
	 * @param message the original message to reply to (contains the sender address)
	 * @param messageId a message ID used to rout the message at the receiver
	 * @param obj the message (as an object)
	 * @param status a return status, sometimes used by reply handling logic
	 */
	template <typename ID, typename T>
	void PostReply(ProtoMessagePtr&& message, ID messageId, const T& obj, uint8_t status = 0)
	{
		if (IsValid())
		{
			if (auto returnAddress = message->GetSender())
			{
				PostReply(std::move(message), *returnAddress, messageId, obj, status);
			}
			else
			{
				message->SendProtoReply(messageId, obj, status);
			}
		}
	}

	/**
	 * Sends a reply to the sender of a message -- the message can be anything
	 * because we make no assumption about what is wrapped in the envelope
	 *
	 * @tparam ID an identifier to be used by the receiver, must cast to uint32_t
	 * @tparam T the message being sent, usually some kind of proto
	 *
	 * @param message the original message to reply to
	 * @param returnAddress the address to reply to
	 * @param messageId a message ID used to rout the message at the receiver
	 * @param obj the message (as an object)
	 * @param status a return status, sometimes used by reply handling logic
	 */
	template <typename ID, typename T>
	void PostReply(PipeMessagePtr&& message, const proto::routing::Address& returnAddress, ID messageId, const T& obj, uint8_t status = 0)
	{
		if (IsValid())
		{
			std::string data = Stuff(returnAddress, messageId, obj);
			message->SendReply(MQMessageId::MSG_ROUTE, &data[0], data.size(), status);
		}
	}

	/**
	 * Checks if the dropbox has a post callback and an address
	 *
	 * @return this dropbox is valid and will send messages
	 */
	bool IsValid() { return m_valid; }

	/**
	 * Removes the mailbox with the same name from the post office
	 */
	void Remove();

private:
	template <typename ID, typename T>
	std::string Stuff(const proto::routing::Address& address, ID messageId, const T& obj)
	{
		return StuffData(address, messageId, obj.SerializeAsString());
	}

	template <typename ID>
	std::string StuffData(const proto::routing::Address& address, ID messageId, const std::string& data)
	{
		proto::routing::Envelope envelope;
		*envelope.mutable_address() = address;

		envelope.set_message_id(static_cast<uint32_t>(messageId));

		proto::routing::Address& ret = *envelope.mutable_return_address();
		ret.set_pid(GetCurrentProcessId());
		ret.set_mailbox(m_localAddress);

		envelope.set_payload(data);

		return envelope.SerializeAsString();
	}

	std::string m_localAddress;
	PostCallback m_post;
	DropboxDropper m_unregister;
	bool m_valid;
};


/**
 * Abstract post office class for handling routing of messages
 *
 * Each application's post office (for instance, MQ and Launcher) will need to implement
 * this class, specifying how to route proto messages. They will also need to define the
 * singleton that the application can use to get the post office to do things like create
 * mailboxes or send mail to other actors (nominally through the launcher).
 *
 * we should assume that everything lives inside an Envelope here. All mail must be
 * in an envelope, no postcards (yet), but we open the Envelope to create ProtoMessages
 * when we put them on the queue
 */
class PostOffice
{
public:
	~PostOffice() {}

	/**
	 * The interface to route a message, to be implemented in the post office instantiation
	 *
	 * @param message the message to route -- it should be in an envelope and have the ID of ROUTE
	 */
	virtual void RouteMessage(PipeMessagePtr&& message) = 0;

	/**
	 * The interface to route a message, to be implemented in the post office instantiation
	 *
	 * @param data the data buffer of the message to route
	 * @param length the length of the data buffer
	 */
	virtual void RouteMessage(const void* data, size_t length) = 0;

	/**
	 * A helper interface to route a message
	 *
	 * @param data a string of data (which embeds its length)
	 */
	void RouteMessage(const std::string& data)
	{
		RouteMessage(&data[0], data.size());
	}

	/**
	 * Creates and registers a mailbox with the post office
	 *
	 * @param localAddress the string address to create the address at
	 * @param receive a callback rvalue that will process messages as they are received in this mailbox
	 * @param mailboxMutator a callback rvalue that will amend the mailbox name universally for handling by the postoffice
	 * @return an dropbox that the creator can use to send addressed messages. will be invalid if it failed to add
	 */
	Dropbox RegisterAddress(const std::string& localAddress, ReceiveCallback&& receive);

	/**
	 * Removes a mailbox from the post office
	 *
	 * @param localAddress the string address that identifies the mailbox to be removed
	 * @return true if the mailbox was removed
	 */
	bool RemoveMailbox(const std::string& localAddress);

	/**
	 * Delivers a message to a local mailbox
	 *
	 * @param localAddress the local address to deliver the message to
	 * @param message the message to send
	 * @param failed a callback for failure (since message is moved)
	 * @return true if routing was successful
	 */
	bool DeliverTo(const std::string& localAddress, PipeMessagePtr&& message, const std::function<void(PipeMessagePtr&&)>& failed = [](const auto&) {});

	/**
	 * Delivers a message to all local mailboxes, optionally excluding self
	 *
	 * @param message the message to send -- only the message ID and the payload is used (the header is rebuilt per message)
	 * @param fromAddress the address to exclude from the delivery
	 */
	void DeliverAll(PipeMessagePtr& message, std::optional<std::string_view> fromAddress = {});

	/**
	 * Processes messages waiting in the queue
	 *
	 * @param howMany how many messages to process (up to)
	 */
	void Process(size_t howMany);

private:
	std::unordered_map<std::string, std::unique_ptr<Mailbox>> m_mailboxes;
};

/**
 * Returns this application's post office singleton
 *
 * @return the single post office that is used in this application
 */
MQLIB_OBJECT PostOffice& GetPostOffice();

} // namespace mq::postoffice
