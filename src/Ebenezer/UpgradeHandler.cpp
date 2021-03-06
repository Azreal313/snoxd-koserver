// Some item ID definitions
#define MIN_ITEM_ID 100000000
#define MAX_ITEM_ID 999999999

#define MAGE_EARRING 310310004
#define WARRIOR_EARRING 310310005
#define ROGUE_EARRING 310310006
#define PRIEST_EARRING 310310007

#define UPGRADE_RUBY_EARRING_MIN 310110005
#define UPGRADE_RUBY_EARRING_MAX 310110007

#define UPGRADE_PEARL_EARRING_MIN 310150005
#define UPGRADE_PEARL_EARRING_MAX 310150007

/**
 * @brief	Packet handler for the assorted systems that
 * 			were deemed to come under the 'upgrade' system.
 *
 * @param	pkt	The packet.
 */
void CUser::ItemUpgradeProcess(Packet & pkt)
{
	uint8 opcode = pkt.read<uint8>();
	switch (opcode)
	{
	case ITEM_UPGRADE:
		ItemUpgrade(pkt);
		break;

	case ITEM_ACCESSORIES:
		ItemUpgradeAccessories(pkt);
		break;

	case ITEM_BIFROST_EXCHANGE:
		BifrostPieceProcess(pkt);
		break;

	case ITEM_UPGRADE_REBIRTH:
		ItemUpgradeRebirth(pkt);
		break;

	case ITEM_SEAL:
		ItemSealProcess(pkt);
		break;

	case ITEM_CHARACTER_SEAL:
		CharacterSealProcess(pkt);
		break;
	}
}

/**
 * @brief	Packet handler for the standard item upgrade system.
 *
 * @param	pkt	The packet.
 */
void CUser::ItemUpgrade(Packet & pkt)
{
	enum UpgradeErrorCodes
	{
		UpgradeFailed		= 0,
		UpgradeSucceeded	= 1,
		UpgradeTrading		= 2,
		UpgradeNeedCoins	= 3,
		UpgradeNoMatch		= 4,
		UpgradeRental		= 5
	};

	Packet result(WIZ_ITEM_UPGRADE, uint8(ITEM_UPGRADE));
	_ITEM_DATA  * pOriginItem;
	_ITEM_TABLE * proto;
	int32 nItemID[10]; int8 bPos[10];
	uint16 sNpcID;
	int8 bType, bResult = UpgradeNoMatch;

	if (isTrading() || isMerchanting())
	{
		bResult = UpgradeTrading;
		goto fail_return;
	}

#if __VERSION >= 1453 // not sure when this occurred, assuming ROFD.
	pkt >> bType; // either preview or upgrade, need to allow for these types
#endif
	pkt >> sNpcID;
	for (int i = 0; i < 10; i++)
		pkt >> nItemID[i] >> bPos[i];

	if (bPos[0] >= HAVE_MAX)
		return;

	pOriginItem = GetItem(SLOT_MAX + bPos[0]);
	if (pOriginItem->nNum != nItemID[0]
		|| (proto = g_pMain->GetItemPtr(nItemID[0])) == nullptr)
		goto fail_return; // error with error code UpgradeNoMatch ("Items required for upgrade do not match")
	else if (pOriginItem->isRented() 
		|| pOriginItem->isSealed()) // unsure if there's another error code for sealed items
	{
		bResult = UpgradeRental;
		goto fail_return;
	}

	// Check the first 5 (+1, first is technically the item we're upgrading) slots. 
	for (int x = 0; x < 5; x++)
	{
		if (bPos[x+1] != -1
			&& (bPos[x+1] >= HAVE_MAX 
				|| nItemID[x+1] != GetItem(bPos[x+1])->nNum))
			goto fail_return;
	}
	
	{ // scoped lock to prevent race conditions
		int nReqOriginItem = nItemID[0] % 1000;
		_ITEM_UPGRADE * pUpgrade = nullptr;
		foreach_stlmap (itr, g_pMain->m_ItemUpgradeArray)
		{
			pUpgrade = itr->second;
			if (pUpgrade->nOriginItem != nReqOriginItem)
				continue;

			if ((nItemID[0] / MIN_ITEM_ID) != pUpgrade->nIndex / 100000
				&& pUpgrade->nIndex < 300000) 
				continue;

			if (pUpgrade->bOriginType != -1 
				&& pUpgrade->nIndex < 200000 && pUpgrade->nIndex >= 100000)
			{
				switch (pUpgrade->bOriginType)
				{
					case 0:
						if (!proto->isStaff()) 
							continue;
						break;

					case 1:
						if (proto->m_bKind != 21)
							continue;
						break;

					case 2:
						if (proto->m_bKind != 22)
							continue;
						break;

					case 3:
						if (proto->m_bKind != 31) 
							continue;
						break;

					case 4:
						if (proto->m_bKind != 32) 
							continue;
						break;

					case 5:
						if (proto->m_bKind != 41) 
							continue;
						break;

					case 6:
						if (proto->m_bKind != 42) 
							continue;
						break;

					case 7:
						if (proto->m_bKind != 51) 
							continue;
						break;

					case 8:
						if (proto->m_bKind != 52) 
							continue;
						break;

					case 9:
						if (proto->m_bKind != 70 && proto->m_bKind != 71) 
							continue;
						break;

					case 10:
						if (proto->m_bKind != 110) 
							continue;
						break;

					case 11:
						if ((nItemID[0] / 10000000) != 19) 
							continue;
						break;

					case 12:
						if (proto->m_bKind != 60) 
							continue;
						break;
				}
			}
			else
			{
				if (((pUpgrade->nIndex >= 300000 || pUpgrade->nIndex < 200000)
					&& (pUpgrade->nIndex >= 400000 || pUpgrade->nIndex < 300000 || proto->m_bSlot - proto->m_bKind == 73))
					|| (proto->m_bKind - proto->m_bSlot) == 8)
					continue;

				break;
			}

			if ((nItemID[0] / MIN_ITEM_ID) != (pUpgrade->nIndex / 100000) 
				&& ((pUpgrade->nIndex / 100000) == 1 
					|| (pUpgrade->nIndex / 100000) == 2))
				continue;

			for (int x = 0; x < 5; x++)
			{
				if (bPos[x+1] != -1
					&& (bPos[x+1] >= HAVE_MAX 
						|| nItemID[x+1] != GetItem(bPos[x+1])->nNum 
						|| nItemID[x+1] != pUpgrade->nReqItem[x]))
					goto fail_return;

				if (pUpgrade->nReqItem[x+1] != 0)
					continue;

				if (pUpgrade->nReqItem[6] == 255 
					|| pUpgrade->nReqItem[6] == proto->m_ItemType)
				{
					if (!hasCoins(pUpgrade->nReqNoah))
					{
						bResult = UpgradeNeedCoins;
						goto fail_return;
					}

					bResult = UpgradeSucceeded;
					break;
				}
			}
		}

		// If we ran out of upgrades to search through, it failed.
		if (bResult != UpgradeSucceeded
			|| pUpgrade == nullptr)
			goto fail_return;

		// Take the required coins
		GoldLose(pUpgrade->nReqNoah);

		// Generate a random number, test if the item burned.
		int rand = myrand(0, myrand(9000, 10000));
		if (pUpgrade->sGenRate <= rand)
		{
			bResult = UpgradeFailed;
			RobItem(nItemID[0], 1); // remove the item
		}
		else
		{
			// Generate the new item ID
			int nNewItemID = pOriginItem->nNum + pUpgrade->nGiveItem;

			// Does this new item exist?
			_ITEM_TABLE * newProto = g_pMain->GetItemPtr(nNewItemID);
			if (newProto == nullptr)
			{ // if not, just say it doesn't match. No point removing the item anyway (like official :/).
				bResult = UpgradeNoMatch;
				goto fail_return;
			}

			// Update the user's item in their inventory with the new item
			pOriginItem->nNum = nNewItemID;

			// Reset the durability also, to the new cap.
			pOriginItem->sDuration = newProto->m_sDuration;

			// Remove all required items, if applicable.
			for (int i = 0; i < 10; i++)
			{
				if (bPos[i] >= HAVE_MAX)
					continue;

				_ITEM_DATA * pItem = GetItem(bPos[i]);
				if (pItem->nNum == 0 
					|| pItem->sCount == 0)
					continue;

				pItem->sCount--;
				if (pItem->sCount == 0)
					memset(pItem, 0, sizeof(pItem));
			}

			// Replace the item ID in the list for the packet
			nItemID[0] = nNewItemID;
		}
	} // end of scoped lock

	result << bResult;
	foreach_array (i, nItemID)
		result << nItemID[i] << bPos[i];
	Send(&result);

	// Send the result to everyone in the area
	// (i.e. make the anvil do its burned/upgraded indicator thing)
	result.Initialize(WIZ_OBJECT_EVENT);
	result << uint8(OBJECT_ANVIL) << bResult << sNpcID;
	SendToRegion(&result);

	return;
fail_return:
	result << bResult;

	// The item data's only sent when not experiencing a general error
	if (bResult != 2)
	{
		foreach_array (i, nItemID)
			result << nItemID[i] << bPos[i];
	}
	Send(&result);
}

/**
 * @brief	Packet handler for the accessory upgrade system.
 *
 * @param	pkt	The packet.
 */
void CUser::ItemUpgradeAccessories(Packet & pkt)
{
}

/**
 * @brief	Packet handler for the Chaotic Generator system
 * 			which is used to exchange Bifrost pieces/fragments.
 *
 * @param	pkt	The packet.
 */
void CUser::BifrostPieceProcess(Packet & pkt)
{
}

/**
 * @brief	Packet handler for the upgrading of 'rebirthed' items.
 *
 * @param	pkt	The packet.
 */
void CUser::ItemUpgradeRebirth(Packet & pkt)
{
}
